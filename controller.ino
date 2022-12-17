#include <Adafruit_NeoPixel.h>

class DisplayConfiguration {
  public:
    virtual void render(*Adafruit_SH1106G sh1106g) = 0;
}

class OledDisplay {
  public:
    Adafruit_SH1106G display;
    OledDisplay() {
      display = Adafruit_SH1106G(128, 64, &SPI1, OLED_DC, OLED_RST, OLED_CS);
    }
    void refresh(DisplayConfiguration display_config) {
      display.clearDisplay();
      display.setCursor(0,0);
      display.println(header_text);

      dc.render(&display);

      // display oled
      display.display();
    }
  
}

class Controller {
    public:
      Key keys[12];
      Adafruit_Neopixel pixels; // key LEDs
      int encoder_pos;
      bool i2c_found[128];
      string header_text;
      // Create the rotary encoder
      RotaryEncoder encoder(PIN_ROTA, PIN_ROTB, RotaryEncoder::LatchMode::FOUR3);
      Controller(string header) {
        // Create the neopixel strip with the built in definitions NUM_NEOPIXEL and PIN_NEOPIXEL
        pixels = Adafruit_NeoPixel(NUM_NEOPIXEL, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
        // Create the OLED display
        
        encoder_pos = encoder.getPosition();
        header_text = header;
      }
      void refresh() { 
        encoder.tick();          // check the encoder
        encoder_pos = encoder.getPosition();
        // show neopixels, increment swirl
        pixels.show();
        
      }

}