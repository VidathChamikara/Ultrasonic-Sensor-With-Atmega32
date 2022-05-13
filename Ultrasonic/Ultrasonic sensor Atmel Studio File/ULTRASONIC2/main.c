/*
 * ULTRASONIC2.c
 *
 * Created: 11/27/2021 11:30:23 AM
 * Author : ASUS
 */ 
#ifndef F_CPU
#define F_CPU 16000000UL // 16 MHz clock speed
#endif


#include <avr/io.h>             // This header file includes the appropriate Input/output definitions for the device
#include <util/delay.h>         // to use delay function we need to include this library
#include <stdlib.h>             // we'll be using itoa() function to convert integer to character array that resides in this library

#define lcd_port PORTD          // we have connected the lcd on port D
#define lcd_data_dir  DDRD      // we're defining the direction of the pins, weather it is input or output
#define rs PD0                  // we need total six pin to show output on the LCD. Datapin 4, 5, 6, and 7 for sending the data to the lcd and Enable and RS pin for controlling
// the behavior of the LCD we have connected RS pin of LCD to port D pin 0
#define en PD1                  // we have connected Enable pin of LCD to port D pin 1

#define US_PORT PORTC           // we have connected the Ultrasonic sensor on port C. to use the ultrasonic we need two pins of the ultrasonic to be connected on port C
#define	US_PIN	PINC            // we need to initialize the pin resistor when we want to take input.
#define US_DDR 	DDRC            // we need data-direction-resistor (DDR) to set the direction of data flow. input or output. we will define it later, now we're just naming it.

#define US_TRIG_POS	PC0         // the trigger pin of ultrasonic sound sensor is connected to port C pin 0
#define US_ECHO_POS	PC1         // the echo pin of the ultrasonic sound sensor is connected to port C pin 1

/*#define led1 PC2                // we've connected four of the LEDs to port C and one LED to port D. The first LED is connected to port C pin 2
#define led2 PC3                // The second LED is connected to port C pin 3 and so on for the rest of the LEDs....
#define led3 PC4
#define led4 PC5
#define led5 PD2*/

#define US_ERROR		-1      // we're defining two more variables two know if the ultrasonic sensor is working or not
#define	US_NO_OBSTACLE	-2

int distance, previous_distance;




void HCSR04Init();
void HCSR04Trigger();

void lcd_command( unsigned char );

void HCSR04Init()
{
	
	// we're setting the trigger pin as output as it will generate ultrasonic sound wave
	US_DDR|=(1<<US_TRIG_POS);
}

void HCSR04Trigger()
{   // this function will generate ultrasonic sound wave for 15 microseconds
	//Send a 10uS pulse on trigger line
	
	US_PORT|=(1<<US_TRIG_POS);	//high
	
	_delay_us(15);				//wait 15uS
	
	US_PORT&=~(1<<US_TRIG_POS);	//low
}

uint16_t GetPulseWidth()
{
	// this function will be used to measure the pulse duration. When the ultra sound echo back after hitting an object
	// the microcontroller will read the pulse using the echo pin of the ultrasonic sensor connected to it.
	
	uint32_t i,result;

	// Section - 1: the following lines of code before the section - 2 is checking if the ultrasonic is working or not
	// it check the echo pin for a certain amount of time. If there is no signal it means the sensor is not working or not connect properly
	for(i=0;i<600000;i++)
	{
		if(!(US_PIN & (1<<US_ECHO_POS)))
		continue;	//Line is still low, so wait
		else
		break;		//High edge detected, so break.
	}

	if(i==600000)
	return US_ERROR;	//Indicates time out
	
	//High Edge Found
	
	// Section -2 : This section is all about preparing the timer for counting the time of the pulse. Timers in microcontrllers is used for timimg operation
	//Setup Timer1
	TCCR1A=0X00;
	TCCR1B=(1<<CS11);	// This line sets the resolution of the timer. Maximum of how much value it should count.
	TCNT1=0x00;			// This line start the counter to start counting time

	// Section -3 : This section checks weather the there is any object or not
	for(i=0;i<600000;i++)                // the 600000 value is used randomly to denote a very small amount of time, almost 40 miliseconds
	{
		if(US_PIN & (1<<US_ECHO_POS))
		{
			if(TCNT1 > 60000) break; else continue;   // if the TCNT1 value gets higher than 60000 it means there is not object in the range of the sensor
		}
		else
		break;
	}

	if(i==600000)
	return US_NO_OBSTACLE;	//Indicates time out

	//Falling edge found

	result=TCNT1;          // microcontroller stores the the value of the counted pulse time in the TCNT1 register. So, we're returning this value to the
	// main function for utilizing it later

	//Stop Timer
	TCCR1B=0x00;

	if(result > 60000)
	return US_NO_OBSTACLE;	//No obstacle
	else
	return (result>>1);
}



void initialize (void)
{
	lcd_data_dir = 0xFF;     // this will set the LCD pins connected on the microcontroller as output
	_delay_ms(15);           // to show data on the LCD we need to send commands first then the data
	lcd_command(0x02);       // this command returns the cursor to the first row and first column position
	lcd_command(0x28);       // please refer to this link to understand meaning of all the commands https://www.electronicsforu.com/technology-trends/learn-electronics/16x2-lcd-pinout-diagram
	lcd_command(0x0c);
	lcd_command(0x06);
	lcd_command(0x01);
	_delay_ms(2);
}

void lcd_command( unsigned char cmnd )
{
	// in order to send command to the lcd first we need to write the command on the data pins. then set the RS pin to zero and enable pin to high
	// then wait for one microseconds and set the enable pin to low, this process repeats again. We're using 4 bit data communication but the data is 8-bit
	// so we will send the data divinding it into two section. Higher 4 bit and lower 4 bit
	// the following lines of codes are used to send higher 4 bits of data
	lcd_port = (lcd_port & 0x0F) | (cmnd & 0xF0);  // this line writes the command on the data pins of the lcd connected to th microcontroller portD pin 4 to 7
	lcd_port &= ~ (1<<rs);
	lcd_port |= (1<<en);
	_delay_us(1);
	lcd_port &= ~ (1<<en);
	
	// wait 200 microseconds
	_delay_us(200);
	
	// send the lower 4 bit of the data
	lcd_port = (lcd_port & 0x0F) | (cmnd << 4);
	lcd_port |= (1<<en);
	_delay_us(1);
	lcd_port &= ~ (1<<en);
	_delay_ms(2);
}

void lcd_clear()
{
	lcd_command (0x01);   // this line clears the LCD screen
	_delay_ms(2);         // waits for two milliseconds
	lcd_command (0x80);   // this line sets the cursor to the row 1 column 1
}


void lcd_print (char *str)
{
	// this function will be used to display the string on the LCD screen
	int i;
	for(i=0; str[i]!=0; i++)
	{
		// we can not send the whole string to the LCD we need to send character by character
		// data sending is same as sending a command. there is one difference, in this case the RS pin will be set to HIGH while the RS pin was set to zero in case of the command sending
		lcd_port = (lcd_port & 0x0F) | (str[i] & 0xF0);
		lcd_port |= (1<<rs);
		lcd_port|= (1<<en);
		_delay_us(1);
		lcd_port &= ~ (1<<en);
		_delay_us(200);
		lcd_port = (lcd_port & 0x0F) | (str[i] << 4);
		lcd_port |= (1<<en);
		_delay_us(1);
		lcd_port &= ~ (1<<en);
		_delay_ms(2);
	}
}



void lcd_setCursor(unsigned char x, unsigned char y){    // this function will be used to set cursor. the place where we want to display the data
	unsigned char adr[] = {0x80, 0xC0};    // the 16x2 LCD has two rows first row has a value of 0x80. So let's say we want to go to the seconds column of first row
	// we just need to send the command with adding 2 with the initial value. So, it will be (0x80 + 2) this is how the code works
	lcd_command(adr[y-1] + x-1);
	_delay_us(100);
}

int main()
{
	DDRA=0xff;
	initialize();           // we're initializing the LCD

	char numberString[4];   // we re defining an array of character. It will be utilized later to store integer to be displayed on the LCD screen

	while(1) {
		
		uint16_t r;
		
		_delay_ms(100);	//Let the LCD Module start


		
		//Set io port direction of sensor
		HCSR04Init();


		/*DDRC |= (1 << led1) | (1 << led2) | ( 1 << led3) | ( 1 << led4);    // we're setting all the leds as output
		DDRD |= (1 << led5);*/                                                 // we need to set the fifth led as output separately because it is on another port
		//PORTC |= 1 << led1;
		while(1)
		{
			
			//Send a trigger pulse
			HCSR04Trigger();               // calling the ultrasonic sound wave generator function

			//Measure the width of pulse
			r=GetPulseWidth();             // getting the duration of the ultrasound took to echo back after hitting the object

			//Handle Errors
			if(r==US_ERROR)                // if microcontroller doesn't get any pulse then it will set the US_ERROR variable to -1
			// the following code will check if there is error then it will be displayed on the LCD screen
			{
				lcd_setCursor(1, 1);      //lcd_setCursor(column, row)
				lcd_print("Error!");
			}
			else 
			{
				
				distance=(r*0.034/2.0);	// This will give the distance in centimeters
				
			   
				
				if (distance != previous_distance)    // the LCD screen only need to be cleared if the distance is changed otherwise it is not required
				{
					lcd_clear();
				}
				
				lcd_setCursor(1, 1);      // set the row and column to display the data
				lcd_print("Water lv = ");
				lcd_setCursor(12, 1);      //lcd_setCursor(column, row)
				itoa(distance, numberString, 10);    // distance is an integer number, we can not display integer directly on the LCD. this line converts integer into array of character
				lcd_print(numberString);
				lcd_setCursor(14, 1);      //set the row to 1 and and column to 14 to display the data
				lcd_print(" cm");
				
				
				
				
				previous_distance = distance;
				_delay_ms(30);
				
				
				 
			}
		}
			
	}
}