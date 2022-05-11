//Declaring all the neccessary global variables
byte screen_buffer[480];
byte serial_input_buffer[100];
int  common_counter;
byte garbage_byte, search_index;

int counterSerialRead=0;
String incomingString="";
int distance = 255;
int angle = 178;
int height = 125;
int batteryLevel = 0;
int packetReceive=0;
unsigned long loop_timer;


void setup()
{
  Serial.begin(9600);               //Enable UART on ATmega328 @ 57600
  //pinMode(0, INPUT);
  pinMode(6,OUTPUT);                 //To SC of MAX7456
  pinMode(10,OUTPUT);                //Not SS of the ATmega328. Needs to be declared as output to get the SPI to work
  pinMode(11, OUTPUT);               //To DIN of MAX7456
  //pinMode(12, INPUT);                //To DOUT of MAX7456
  pinMode(13,OUTPUT);                //To SCK of MAX7456
  
  digitalWrite(6,HIGH);              //Disable the MAX7456
  
  SPCR = (1<<SPE)|(1<<MSTR);         //Set SPI enable and SPI Master bit
  delay(250); 
  
  /*Reset the MAX7456*/
  digitalWrite(6,LOW);               //Enable the MAX7456
  spi_transfer(0x00, 0x42);          //Write 0b01000010 (PAL & software reset bits) to register h00 (Video Mode 0)
  digitalWrite(6,HIGH);              //disable the MAX7456
  delay(250);                        //Wait for the MAX7456 to complete reset
  

  digitalWrite(6,LOW);               //Enable the MAX7456
  /*Horizontal position offset*/
  spi_transfer(0x02, 0x2C);          //0x20 = no correction, min = 0x00 (-32 pixels), max = 0x3F (+31 pixels)
  /*Vertical position offset*/
  spi_transfer(0x03, 0x16);          //0x10 = no correction, min = 0x00 (+16 pixels), max = 0x1F (-15 pixels)
  /*Enable MAX7456*/
  spi_transfer(0x00, 0x48);          //Write 0b01001000 (PAL & enable OSD) to register h00 (Video Mode 0)
  digitalWrite(6,HIGH);              //disable device
  
  /*Clear the screen buffer array*/
  for (common_counter = 0; common_counter < 480; common_counter++){
    screen_buffer[common_counter] = 0x00;
  }
}

void loop()
{
  loop_timer = millis();
  while(loop_timer + 900>millis()){//Receiving data from the Arduino through the Serial Monitor and sorting it in 4 seperate global variables
    while(Serial.available()){
      char incoming = (char)Serial.read();
      if(incoming=='/' || packetReceive == 1){
        packetReceive=1;
      if(isDigit(incoming) && incoming != ';'){
        incomingString += (char)incoming; 
      }
      if(incoming ==';'){
        if (counterSerialRead==0){
          distance = (unsigned long)incomingString.toInt();
          counterSerialRead++;
          incomingString="";
        }else if (counterSerialRead==1){
          angle = (unsigned long)incomingString.toInt();
          counterSerialRead++;
          incomingString="";
        }else if (counterSerialRead==2){
          height = (unsigned long)incomingString.toInt();
          counterSerialRead++;
          incomingString="";
        }else{
          batteryLevel = (unsigned long)incomingString.toFloat();
          counterSerialRead=0;
          packetReceive=0;
          incomingString="";
        }
      }
    }
   }
  }
    
  /*Clear the on screen display buffer*/
  for (common_counter = 0; common_counter < 480; common_counter++)
  {
    screen_buffer[common_counter] = 0x00;
  }
  
  /*Display the number of degrees the drone should rotate to reach the beacon*/
  screen_buffer[42] = convert_dec(angle/100);
  screen_buffer[43] = convert_dec((angle - ((angle/100)*100)) / 10);
  screen_buffer[44] = convert_dec((angle - ((angle/100)*100)) % 10);
  
  /*Display height and the meter symbol*/
  screen_buffer[206] = 0xC6; //Meter symbol
  screen_buffer[203] = convert_dec(height/100);
  screen_buffer[204] = convert_dec((height - ((height/100)*100)) / 10);
  screen_buffer[205] = convert_dec((height - ((height/100)*100)) % 10);
  
  /*Display distance and the meter symbol*/
  screen_buffer[393] = 0xC3; //Distance symbol
  screen_buffer[394] = convert_dec(distance/1000);
  screen_buffer[395] = convert_dec(distance / 100);
  screen_buffer[396] = convert_dec(distance / 10);
  screen_buffer[397] = convert_dec(distance % 10);
  screen_buffer[398] = 0xC6; //Meter symbol
  
  /*Display battery voltage*/
  screen_buffer[50] = 0xC2; //Battery symbol
  screen_buffer[51] = convert_dec(batteryLevel / 100);
  screen_buffer[52] = convert_dec(batteryLevel / 10);
  screen_buffer[53] = 0xC9; //Dot symbol
  screen_buffer[54] = convert_dec(batteryLevel % 10);
  
  /*Write the new screen*/
  write_new_screen();
}

byte spi_transfer(byte adress, byte data)
{
  SPDR = adress;                    //Load SPI data register
  while (!(SPSR & (1<<SPIF)));      //Wait until the byte is send
  SPDR = data;                      //Load SPI data register
  while (!(SPSR & (1<<SPIF)));      //Wait until the byte is send
}

int convert_dec (int decimal)
{
  int convert_dec;
  if (decimal == 0) convert_dec = 0xD0;
  if (decimal == 1) convert_dec = 0xD1;
  if (decimal == 2) convert_dec = 0xD2;
  if (decimal == 3) convert_dec = 0xD3;
  if (decimal == 4) convert_dec = 0xD4;
  if (decimal == 5) convert_dec = 0xD5;
  if (decimal == 6) convert_dec = 0xD6;
  if (decimal == 7) convert_dec = 0xD7;
  if (decimal == 8) convert_dec = 0xD8;
  if (decimal == 9) convert_dec = 0xD9;
  return (convert_dec);
}

void write_new_screen()
{
  int local_count;
  
  local_count = 479; //The number of possible spaces that can be filled with the MAX7456
  
  digitalWrite(6,LOW); //Enable the MAX7456
  spi_transfer(0x04, 0x04); //Clear the display
  digitalWrite(6,HIGH); //Disable the MAX7456

  digitalWrite(6,LOW); //Enable the MAX7456
  spi_transfer(0x00, 0x40); //Disable the on screen display
  spi_transfer(0x04, 0x01); //Auto increment mode for writing the display characters
  spi_transfer(0x05, 0x00); //Set start address high to the position 0
  spi_transfer(0x06, 0x00); //Set start address low to the position 0

  common_counter = 0;
  while(local_count) //Display data on the whole display
  {
    spi_transfer(0x07, screen_buffer[common_counter]); //Load the character in the next buffer of the MAX7456
    common_counter++;
    local_count--;
  }

  spi_transfer(0x07, 0xFF); //Disable the auto increment mode

  spi_transfer(0x00, 0x4C); // Turn on the OSD
  digitalWrite(6,HIGH); //Disable the MAX7456
}