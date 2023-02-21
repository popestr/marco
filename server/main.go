package main

import (
	"encoding/binary"
	"errors"
	"fmt"
	"log"
	"math/rand"
	"strconv"
	"strings"

	"github.com/atotto/clipboard"
	"go.bug.st/serial"
	"go.bug.st/serial/enumerator"
)

const (
	CLIPBOARD              = 0x0
	CLIPBOARD_PRIME        = 0x1
	CLIPBOARD_CANCEL_PRIME = 0x2
	CLIPBOARD_REQUEST_CLIP = 0x3

	OLED = 0xE

	KEYS = 0xF

	ARD_INSTRUCTION_PREFIX        = "[ARD::0x"
	ARD_INSTRUCTION_PREFIX_LENGTH = len(ARD_INSTRUCTION_PREFIX)
)

type Instruction struct {
	instructionCode byte
	arg1            byte
	arg2            byte
	arg3            byte
	additionalArgs  string
}

func NewInstructionFromSerial(serialInput string) (*Instruction, bool, error) {
	serialInput = strings.TrimSuffix(serialInput, "\r\n")
	fmt.Println(serialInput)
	var sanitizedInput string
	if prefixIndex := strings.Index(serialInput, ARD_INSTRUCTION_PREFIX); prefixIndex == 0 {
		sanitizedInput = serialInput[prefixIndex+ARD_INSTRUCTION_PREFIX_LENGTH : len(serialInput)-1]
		fmt.Println(sanitizedInput)
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
	return []byte(fmt.Sprintf("[MTH::%s]%s\r\n", i.hexFormatted(), i.additionalArgs))
}

var (
	port serial.Port
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

func setKeyColor(keyNo byte, hexCode string) error {
	colorSend := Instruction{
		instructionCode: KEYS,
		arg1:            keyNo,
		additionalArgs:  strings.ToUpper(hexCode),
		arg3:            1,
	}
	return sendSerial(colorSend.serialize())
}

func setOledText(lineNo byte, inverted byte, text string) error {
	textSend := Instruction{
		instructionCode: OLED,
		arg1:            inverted,
		arg2:            lineNo,
		arg3:            1,
		additionalArgs:  text,
	}
	return sendSerial(textSend.serialize())
}

func delegate(i *Instruction) error {
	switch i.instructionCode {
	case CLIPBOARD:
		switch i.arg2 {
		case CLIPBOARD_PRIME:
			setKeyColor(i.arg1, "ffffff")
			break
		case CLIPBOARD_CANCEL_PRIME:
			setKeyColor(i.arg1, "000000")
			clip, err := clipboard.ReadAll()
			if err != nil {
				return err
			}
			setOledText(1, 0, clip)
			break
		case CLIPBOARD_REQUEST_CLIP:
			clip, err := clipboard.ReadAll()
			if err != nil {
				return err
			}
			setOledText(1, 0, clip)
		}
		break
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

	for {
		buff := make([]byte, 100)
		bytePos := 0
		for !strings.Contains(string(buff), "\r\n") {
			n, err := port.Read(buff[bytePos:])
			if err != nil {
				log.Fatal(err)
				break
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
				//
			} else if !valid {
				continue
			}
			fmt.Println(i)
			fmt.Println(i.hexFormatted())
			setKeyColor(byte(rand.Intn(7)), "ffffff")
			setOledText(byte(rand.Intn(7)), 1, "yeubh")
			delegate(i)
		}
	}
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
