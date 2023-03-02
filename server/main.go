package main

import (
	"bytes"
	"encoding/binary"
	"errors"
	"fmt"
	"io"
	"log"
	"os"
	"regexp"
	"strconv"
	"strings"
	"time"

	"github.com/atotto/clipboard"
	"go.bug.st/serial"
	"go.bug.st/serial/enumerator"
)

const (
	NUM_KEYS = 12

	EVENT   = 0x0
	KEYUP   = 0x0001
	KEYDOWN = 0x0002
	KEYHOLD = 0x0003

	OLED = 0xE

	KEYS = 0xF

	ARD_INSTRUCTION_PREFIX        = "[ARD::0x"
	ARD_INSTRUCTION_PREFIX_LENGTH = len(ARD_INSTRUCTION_PREFIX)
	CLIP_DIRECTORY                = "clipboards"
	CLIPBOARD_ACTIVE_COLOR        = "21ff4a"
	CLIPBOARD_INACTIVE_COLOR      = "000000"
)

type Key struct {
	// how many times in a row this key has been pressed
	sequentialPressCount uint

	// if this key has seen KEYDOWN but not KEYUP
	isPressed bool

	// KEYHOLD messages are not synced perfectly, and limited to one bit
	// it cycles 0->F->0, so we keep this to keep a (mostly) accurate sum
	lastDurationValue byte

	// this is the sum, see above comment
	pressDuration uint
}

func (k *Key) handle(i *Instruction) {
	switch i.arg2 {
	case KEYDOWN:
		k.sequentialPressCount++
		k.isPressed = true
	case KEYHOLD:
		var extra byte
		if i.arg3 < k.lastDurationValue {
			// if we rolled over to next cycle, account for that
			extra = 0xF + 1
		}
		k.pressDuration += uint(i.arg3 + extra - k.lastDurationValue)
	case KEYUP:
		k.isPressed = false
	default:
		return
	}
}

func (k *Key) reset() {
	k.isPressed = false // in case we missed a KEYDOWN somehow
	k.sequentialPressCount = 0
	k.pressDuration = 0
}

type Instruction struct {
	instructionCode byte
	arg1            byte
	arg2            byte
	arg3            byte
	additionalArgs  string
}

func NewInstructionFromSerial(serialInput string) (*Instruction, bool, error) {
	serialInput = strings.TrimSuffix(serialInput, "\r\n")
	// fmt.Printf("parsing: %s\n", serialInput)
	var sanitizedInput string
	if prefixIndex := strings.Index(serialInput, ARD_INSTRUCTION_PREFIX); prefixIndex == 0 {
		sanitizedInput = serialInput[prefixIndex+ARD_INSTRUCTION_PREFIX_LENGTH : len(serialInput)-1]
		// fmt.Println(sanitizedInput)
	} else {
		return nil, false, nil
	}
	if len(sanitizedInput) != 4 {
		return nil, false, fmt.Errorf("unexpected bit width: %s", sanitizedInput)
	}
	asInt, err := strconv.ParseInt(sanitizedInput, 16, 64)
	if err != nil {
		return nil, false, err
	}

	return &Instruction{
		instructionCode: byte((asInt & 0xF000) >> (4 * 3)),
		arg1:            byte((asInt & 0x0F00) >> (4 * 2)),
		arg2:            byte((asInt & 0x00F0) >> 4),
		arg3:            byte(asInt & 0x000F),
	}, true, nil
}

func (i *Instruction) toBytes() []byte {
	return []byte{i.instructionCode<<4 | i.arg1, i.arg2<<4 | i.arg3}
}

func (i *Instruction) asInt() uint16 {
	return uint16(binary.BigEndian.Uint16(i.toBytes()))
}

func (i *Instruction) hexFormatted() string {
	return fmt.Sprintf("0x%04X", i.asInt())
}

func (i *Instruction) serialize() []byte {
	return []byte(fmt.Sprintf("[MTH::%s]%s\r\n", i.hexFormatted(), spacingRegex.ReplaceAllString(i.additionalArgs, " ")))
}

var (
	port             serial.Port
	clipboardManager *ClipboardManager
	spacingRegex     = regexp.MustCompile(`[^\S ]`)
	keys             [NUM_KEYS]*Key
	lastKeyIndex     = 0
)

func sendSerial(message []byte) error {
	n, err := port.Write(message)
	if err != nil {
		return err
	}
	if n != len(message) {
		return errors.New("bytes transmitted not of expected length")
	}
	return nil
}

// setKeyColor sets the color of a key's LED.
// Indexed from 0-11 for an Adafruit RP2040 Macropad.
func setKeyColor(keyIndex byte, hexCode string) error {
	colorInstruction := Instruction{
		instructionCode: KEYS,
		arg1:            keyIndex,
		additionalArgs:  strings.ToUpper(hexCode),
		arg3:            1,
	}
	return sendSerial(colorInstruction.serialize())
}

func setOledText(lineNo byte, inverted byte, text string) error {
	textInstruction := Instruction{
		instructionCode: OLED,
		arg1:            inverted,
		arg2:            lineNo,
		arg3:            1,
		additionalArgs:  text,
	}
	return sendSerial(textInstruction.serialize())
}

func clearOled() error {
	clearInstruction := Instruction{
		instructionCode: OLED,
		arg3:            0,
	}
	return sendSerial(clearInstruction.serialize())
}

func clearOledLines(startLine byte, endLine byte) error {
	clearInstruction := Instruction{
		instructionCode: OLED,
		arg1:            startLine,
		arg2:            endLine,
		arg3:            0,
	}
	return sendSerial(clearInstruction.serialize())
}

type Clipboard struct {
	filepath string
	value    string
}

type ClipboardManager struct {
	clipboards          [NUM_KEYS]*Clipboard
	previousIndex       int
	previousUpdateIndex int
}

func NewClipboardManager(directory string) *ClipboardManager {
	result := &ClipboardManager{
		clipboards: [NUM_KEYS]*Clipboard{},
	}
	err := createDirIfNotExists(directory)
	if err != nil {
		log.Fatal(err)
	}
	for i := 0; i < NUM_KEYS; i++ {
		filepath := directory + "/clipboard" + fmt.Sprint(i) + ".txt"
		if err != nil {
			log.Fatal(err)
		}
		result.clipboards[i] = &Clipboard{
			filepath: filepath,
		}
		value, err := result.getClip(i)
		if err != nil {
			log.Fatal(err)
		}
		result.clipboards[i].value = value
	}
	return result
}

func (cm *ClipboardManager) setClip(index int) (string, error) {
	if index < 0 || index > NUM_KEYS-1 {
		return "", fmt.Errorf("index %d out of bounds", index)
	}
	if cm.clipboards[index].filepath == "" {
		return "", fmt.Errorf("clipboard %d has no path", index)
	}

	file, _, err := openOrCreate(cm.clipboards[index].filepath)
	if err != nil {
		return "", err
	}
	defer file.Close()

	clipboardContents, err := clipboard.ReadAll()
	if err != nil {
		return "", err
	}

	_, err = file.Write([]byte(clipboardContents))
	if err != nil {
		return "", err
	}

	cm.clipboards[index].value = clipboardContents
	cm.previousIndex = index
	cm.previousUpdateIndex = index
	return clipboardContents, nil
}

func (cm *ClipboardManager) getClip(index int) (string, error) {
	if index < 0 || index > NUM_KEYS-1 {
		return "", fmt.Errorf("index %d out of bounds", index)
	}
	if cm.clipboards[index].filepath == "" {
		return "", fmt.Errorf("clipboard %d has no path", index)
	}
	file, created, err := openOrCreate(cm.clipboards[index].filepath)
	if err != nil {
		return "", err
	}
	if created {
		return "", nil
	}
	buf := bytes.NewBuffer(nil)
	_, err = io.Copy(buf, file)
	if err != nil {
		return "", err
	}

	cm.previousIndex = index
	cm.previousUpdateIndex = index
	return buf.String(), file.Close()
}

func (cm *ClipboardManager) hasClip(index int) bool {
	cm.previousIndex = index
	return cm.clipboards[index] != nil && cm.clipboards[index].value != ""
}

func (cm *ClipboardManager) deleteClip(index int) error {
	if cm.hasClip(index) {
		cm.clipboards[index].value = ""
		cm.previousIndex = index
		cm.previousUpdateIndex = index
		return os.Truncate(cm.clipboards[index].filepath, 0)
	}
	return fmt.Errorf("attempted to delete non-existent clipboard %d", index)
}

func (cm *ClipboardManager) updateLEDs() {
	for index := range cm.clipboards {
		if cm.hasClip(index) {
			setKeyColor(byte(index), CLIPBOARD_ACTIVE_COLOR)
		} else {
			setKeyColor(byte(index), CLIPBOARD_INACTIVE_COLOR)
		}
	}
}

func createDirIfNotExists(path string) error {
	if stat, err := os.Stat(path); os.IsNotExist(err) {
		return os.Mkdir(path, os.ModePerm)
	} else if err != nil {
		return err
	} else if !stat.IsDir() {
		return fmt.Errorf("path %s exists but is not a directory", path)
	}
	// directory already exists
	return nil
}

func openOrCreate(path string) (*os.File, bool, error) {
	if stat, err := os.Stat(path); os.IsNotExist(err) {
		file, err := os.Create(path)
		return file, true, err
	} else if stat.IsDir() {
		return nil, false, fmt.Errorf("path %s exists but is a directory", path)
	}
	file, err := os.OpenFile(path, os.O_RDWR, 0660)
	return file, false, err
}

func delegate(i *Instruction) error {
	switch i.instructionCode {
	case EVENT:
		switch i.arg2 {
		case KEYUP, KEYDOWN, KEYHOLD:
			{
				keyIndex := int(i.arg1)
				if keyIndex != lastKeyIndex {
					keys[lastKeyIndex].reset()
				}
				currKey := keys[keyIndex]
				currKey.handle(i)
				fmt.Println(currKey)
				setOledText(0, 1, fmt.Sprintf("clipboard %d:", keyIndex))
				holdDurationMs := currKey.pressDuration * 100
				if !currKey.isPressed { // we have seen KEYUP
					if holdDurationMs < 1000 {
						if clipboardManager.hasClip(keyIndex) {
							clip, err := clipboardManager.getClip(keyIndex)
							if err != nil {
								return err
							}

							if currKey.sequentialPressCount == 1 {
								err = clearOledLines(1, 0)
								if err != nil {
									return err
								}
								setOledText(2, 0, clip)
							} else if currKey.sequentialPressCount == 2 {
								err = clearOledLines(1, 0)
								if err != nil {
									return err
								}
								clipboard.WriteAll(clip)
								setOledText(2, 0, "successfully copied!")
								setOledText(4, 0, clip)
								clipboardManager.previousUpdateIndex = -1
							}
						} else {
							err := clearOledLines(1, 0)
							if err != nil {
								return err
							}
							setOledText(2, 0, "nothing here =(")
							setOledText(4, 0, "Hold for 1s to copy")
							setOledText(5, 0, "from host clipboard.")
						}
					} else if holdDurationMs >= 1000 {
						clearOledLines(1, 0)
						if !clipboardManager.hasClip(keyIndex) {
							clip, err := clipboardManager.setClip(keyIndex)
							if err != nil {
								return err
							}
							setOledText(2, 0, "set to:")
							setOledText(4, 0, clip)
						} else {
							err := clipboardManager.deleteClip(keyIndex)
							if err != nil {
								return err
							}
							setOledText(1, 0, "deleted clip!")
						}
						currKey.reset()
					}
				}
				clipboardManager.updateLEDs()
				lastKeyIndex = keyIndex
			}
		}
	}
	return nil
}

func main() {
	ports, err := enumerator.GetDetailedPortsList()
	if err != nil {
		log.Fatal(err)
	}
	portName, err := selectPortByKnownSerial(ports)
	if err != nil {
		log.Fatal(err)
	}
	log.Println(portName)

	mode := &serial.Mode{
		BaudRate: 115200,
	}
	port, err = serial.Open(portName, mode)
	if err != nil {
		log.Fatal(err)
	}

	for i := 0; i < NUM_KEYS; i++ {
		keys[i] = &Key{}
	}

	clearOled()
	setOledText(0, 1, "-MULTICLIPBOARD MODE-")
	setOledText(2, 0, "registered with host!")
	setOledText(4, 0, "press a key to start.")
	setOledText(6, 0, "          =)         ")

	clipboardManager = NewClipboardManager(CLIP_DIRECTORY)
	clipboardManager.updateLEDs()

	for {
		buff := make([]byte, 165)
		bytePos := 0
		for !strings.Contains(string(buff), "\r\n") {
			n, err := port.Read(buff[bytePos:])
			if err != nil {
				port, err = revivePortWithRetry(portName, mode, 3, 3*time.Second)
				if err != nil {
					log.Fatal(err)
				}
				continue
			}
			if n == 0 {
				fmt.Println("\nEOF")
				break
			}
			bytePos += n
		}
		for _, instructionBytes := range strings.Split(string(buff[:bytePos]), "\r\n") {
			if instructionBytes == "" {
				continue
			}
			i, valid, err := NewInstructionFromSerial(instructionBytes)
			if err != nil {
				log.Fatal(err)
			} else if !valid {
				continue
			}
			fmt.Println(i)
			fmt.Println(i.hexFormatted())
			err = delegate(i)
			if err != nil {
				log.Fatal(err)
			}
		}
	}
}

func revivePortWithRetry(portName string, mode *serial.Mode, retries int, interval time.Duration) (serial.Port, error) {
	totalRetries := 0
	var err error

	fmt.Printf("Unable to connect to %s, retrying in %d seconds...\n", portName, int(interval.Seconds()))
	for totalRetries < retries {
		port, err := serial.Open(portName, mode)
		if err != nil {
			totalRetries++
			time.Sleep(interval)
		} else {
			fmt.Printf("Reconnected to %s successfully!\n", portName)
			return port, nil
		}
	}
	return nil, err

}

func selectPortByKnownSerial(ports []*enumerator.PortDetails) (string, error) {
	if len(ports) == 0 {
		return "", errors.New("no ports available")
	}
	for _, port := range ports {
		if port.SerialNumber == "DF60BCA003370F35" {
			return port.Name, nil
		}
	}
	return "", fmt.Errorf("no ports matched known serial, highest COM port is %s", ports[len(ports)-1].Name)
}
