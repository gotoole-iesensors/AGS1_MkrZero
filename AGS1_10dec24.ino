#define Timed_Initial_Zero  
#define Timed_Initial_Zero_Period 3600   //Zero set after 3600s
#define Continous_Output_Period   5     //Continuous output interval of 5s

#define AGS_Vdd     3.3                 //(Dependent on voltage supply to AGS1)
#define ADC_Vref    3.3                 //(Dependent on Arduino board)
#define ADC_Range   4096                //(Dependent on Arduino board)
#define ADC_Average 60                  //(1-300)
#define Rgain       100E3               //(Varies by AGS board gas type)

#define Sensor_Serial_Number {'0', '4', '1', '1', '1', '7', '0', '1', '1', '2', '4', '2', '\0'} 
#define nA_per_PPM  3.93                //Use QR reader to get Sensor_Serial_Number and nA_per_PPM
#define ADC0_Zero   31022               //Adjust this manually
#define ADC0_OC     30987               //Adjust this manually
#define ADC1_Zero   30374               //Adjust this manually
#define N           1000                //Adjust this manually

void setup() 
{
   Serial.begin(9600);
   delay(3000);

  Serial.println("\r\r\r\r\r\r\r\r");
  Serial.println("************************************************************");
  Serial.println("Enter return to perform single read");
  Serial.println("Enter 'e' to read EEPROM");
  Serial.println("Enter 'C' to toggle/untoggle continuous output");
  Serial.println("Enter 'O' to set open circuit measurement");
  Serial.println("Enter 'Z' to set zero circuit measurement");
  Serial.println("************************************************************");
  Serial.println("Serial, PPB, Tx100, Hx100, ADC_Raw, T_Raw, H_Raw");
}

void loop() 
{  
  unsigned long int i;

  char UART_Command;
  unsigned long int LastMillis;
  unsigned int Continous_Output_Counter;
  bool Continous_Output = false;
  bool Zero_Timer_Triggered = false;

  unsigned int ADC0Result_[300];
  unsigned long int ADC0Result;           //Vgas   
  unsigned long int ADC0Result_OC = ADC0_OC;
  unsigned long int ADC0Result_Zero = ADC0_Zero;  

  unsigned long int ADC1Result;           //Vtemp
  unsigned long int ADC1Result_Zero = ADC1_Zero;

  char Serial_Number[13] = Sensor_Serial_Number;
  float nA;
  float Zcf;
  float n_factor = N;
  float PPB;
  float Temperature;
  float Temperature_Zero = ((float)87 / (float)AGS_Vdd * ((float)ADC1Result_Zero / ((float)ADC_Range * 16) * (float)ADC_Vref)) - (float)18;
  float Relative_Humidity = 50;           //No RH sensor availiable on AGS1
  
  i = 0;
  while(i < ADC_Average)
  {
    ADC0Result_[i] = ADC0Result_Zero;
    i++;
  }

  Continous_Output_Counter = 0;
  UART_Command = '\0';
  LastMillis = millis();
  while(1)
  {
    if((millis() - LastMillis) > 1000)
    {
      LastMillis = millis();
      
      i = ADC_Average - 1;
      while(i > 0)
      {
        ADC0Result_[i] = ADC0Result_[i - 1];
        i--;
      }
      
      analogRead(A0);
      ADC0Result = 0;
      i = 0;
      while(i < 4097)
      {
        ADC0Result = ADC0Result + analogRead(A0);
        i++;
      }
      ADC0Result_[0] = ADC0Result >> 8;
  
      ADC0Result = 0;
      i = 0;
      while(i < ADC_Average)
      {
        ADC0Result = ADC0Result + ADC0Result_[i];
        i++;
      }
      ADC0Result = ADC0Result / ADC_Average;
  
      analogRead(A1);   
      ADC1Result = 0;
      i = 0;
      while(i < 256)
      {
        ADC1Result = ADC1Result + analogRead(A1);
        i++;
      }
      ADC1Result = ADC1Result >> 4;

      Temperature = ((float)87 / (float)AGS_Vdd * ((float)ADC1Result / ((float)ADC_Range * 16) * (float)ADC_Vref)) - (float)18;
      nA = ((float)ADC0Result - (float)ADC0Result_OC) / ((float)ADC_Range * 16) * (float)ADC_Vref / (float)Rgain * 1E9;
      Zcf = ((float)ADC0Result_Zero - (float)ADC0Result_OC) / ((float)ADC_Range * 16) * (float)ADC_Vref / (float)Rgain * 1E9;    
      Zcf = Zcf * exp((Temperature - Temperature_Zero) / n_factor);  
      nA = nA - Zcf;  
      PPB = nA / nA_per_PPM * 1E3;
//      PPB = ((float)ADC0Result - (float)ADC0Result_Zero) / ((float)ADC_Range * 16) * (float)ADC_Vref / (float)Rgain * (float)1E9 / (float)nA_per_PPM * (float)1E3;

      Continous_Output_Counter++;
      if(Continous_Output && (Continous_Output_Counter >= Continous_Output_Period))
      {
        Continous_Output_Counter = 0;
        
        Serial.print(Serial_Number);
        Serial.print(", ");
        Serial.print(PPB, 0);
        Serial.print(", ");
        Serial.print((Temperature * 100), 0);
        Serial.print(", ");
        Serial.print((Relative_Humidity * 100), 0);
        Serial.print(", ");
        Serial.print(ADC0Result);
        Serial.print(", ");
        Serial.print(ADC1Result);
        Serial.print(", ");
        Serial.print("32768");
        Serial.println("");
      }      
    }
  
#ifdef Timed_Initial_Zero
    if((millis() > (Timed_Initial_Zero_Period * 1000)) && (!Zero_Timer_Triggered)) 
    {
      Zero_Timer_Triggered = true;
      ADC0Result_Zero = ADC0Result;
      ADC1Result_Zero = ADC1Result;
      Temperature_Zero = ((float)87 / (float)AGS_Vdd * ((float)ADC1Result_Zero / ((float)ADC_Range * 16) * (float)ADC_Vref)) - (float)18;
      Serial.println("************************************************************");
      Serial.print("Zero Set:"); 
      Serial.print(" ADC0_Zero = ");
      Serial.print(ADC0Result_Zero);
      Serial.print(" ADC1_Zero = ");
      Serial.println(ADC1Result_Zero);
      Serial.println("************************************************************");
    }
#endif

    while (Serial.available() > 0) 
    {
      UART_Command = Serial.read();
      if(UART_Command == '\r')
      {
        UART_Command = '\0';

        Serial.print(Serial_Number);
        Serial.print(", ");
        Serial.print(PPB, 0);
        Serial.print(", ");
        Serial.print((Temperature * 100), 0);
        Serial.print(", ");
        Serial.print((Relative_Humidity * 100), 0);
        Serial.print(", ");
        Serial.print(ADC0Result);
        Serial.print(", ");
        Serial.print(ADC1Result);
        Serial.print(", ");
        Serial.print("32768");
        Serial.println("");
      }
      if(UART_Command == 'e')
      {
        UART_Command = '\0';
        
        Serial.println("************************************************************");
        Serial.print("ADC0_Zero = ");
        Serial.print(ADC0Result_Zero);
        Serial.print(" ADC1_Zero = ");
        Serial.println(ADC1Result_Zero);

        Serial.print("ADC0_OC   = ");
        Serial.print(ADC0Result_OC);
        Serial.print(" N         = ");
        Serial.println(n_factor);
        Serial.println("************************************************************");
      }
      if(UART_Command == 'C')
      {
        UART_Command = '\0';

        if(Continous_Output)
        {
          Continous_Output = false;
        }
        else
        {
          Continous_Output = true;
        }
      }
      if(UART_Command == 'O')
      {
        UART_Command = '\0';

        Serial.println("************************************************************");
        ADC0Result_OC = ADC0Result;
        Serial.print("Open Circuit Set:"); 
        Serial.print(" ADC0_OC = ");
        Serial.println(ADC0Result_OC);
        Serial.println("************************************************************");
      }
      if(UART_Command == 'Z')
      {
        UART_Command = '\0';

        ADC0Result_Zero = ADC0Result;
        ADC1Result_Zero = ADC1Result;
        Temperature_Zero = ((float)87 / (float)AGS_Vdd * ((float)ADC1Result_Zero / ((float)ADC_Range * 16) * (float)ADC_Vref)) - (float)18;
        Serial.println("************************************************************");
        Serial.print("Zero Set:"); 
        Serial.print(" ADC0_Zero = ");
        Serial.print(ADC0Result_Zero);
        Serial.print(" ADC1_Zero = ");
        Serial.println(ADC1Result_Zero);
        Serial.println("************************************************************");
      }
    }
  }
}