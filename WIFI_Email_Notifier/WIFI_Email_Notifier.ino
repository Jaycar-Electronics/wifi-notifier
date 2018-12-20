#define WIFI Serial1
#define DEBUG Serial

#define SSIDNAME "********"
#define SSIDPWD "********"
#define HOSTNAME "smtp.virginbroadband.com.au"
#define SERVERNAME "virginbroadband.com.au"
#define HOSTPORT "25"
#define RCPTEMAIL "*****@virginbroadband.com.au"
#define SUBJECTLINE "Subject: Temperature Alert from Arduino"
char num[15]="";
long coretemp;
long rpttimeout;    //time between reporting in milliseconds
#define RPTDELAY 900000UL

void setup() {
  WIFI.begin(115200);   //start serial ports
  DEBUG.begin(115200);
  while((!DEBUG)&&(millis()<10000));    //wait for serial port, or start anyway after 10 seconds
  wifiinit();     //send starting settings- find AP, connect and set to station mode only
  DEBUG.println("XC4614 is ready and waiting...");
  DEBUG.println("Press Enter to manually trigger an email.");
  rpttimeout=millis()-RPTDELAY;     //set timeout so report can occur immediately if necessary
}

void loop() {
  coretemp=analogRead(A0)/10-24;      //approximate temperature conversion- is good for +-2 degrees below 60
  while(DEBUG.available()){
    int d=DEBUG.read();
    if(d==13){        //if enter pressed, trigger email
      doemail();
    }
  }
  if((coretemp>35)&&(rpttimeout+RPTDELAY<millis())){   //if it's been more than delay since last report
    doemail();
    rpttimeout=millis();
  }
}

void doemail(){
  DEBUG.println("OK, working...");
  int c;
  for(int i=0;i<3;i++){     //maximum of three attempts
    c=processcmd();         //if result is not zero, email was sent ok
    DEBUG.print("Attempt ");
    DEBUG.print(i+1);
    DEBUG.print(" result:");
    DEBUG.println(c);
    if(c){
      DEBUG.println("Email sent successfully.");
      return;
      }      //finish if successful      
    delay(2000);        //wait a bit
  }
  DEBUG.println("Email not sent.");
}

int processcmd(){
  DEBUG.print("Connect:");
  int c;
  c=WIFIcmd("AT+CIPSTART=\"TCP\",\""  HOSTNAME  "\"," HOSTPORT  ,"CONNECT\r\n",10000);        //start connection
  DEBUG.println(c);
  if(!c){DEBUG.println("Could not connect");return 0;}                                        //connection timed out
  WIFIsend("HELO " SERVERNAME "\r\n");      //connect to host
  if(!WIFIreceive("250")){WIFIclose;return 0;}    //
  WIFIsend("MAIL FROM:<" RCPTEMAIL ">\r\n");      //sender (same as recipient)
  if(!WIFIreceive("250")){WIFIclose;return 0;}    //
  WIFIsend("RCPT TO:<" RCPTEMAIL ">\r\n");      //recipient (same as recipient)
  if(!WIFIreceive("250")){WIFIclose;return 0;}    //
  WIFIsend("DATA\r\n");                         //content of email starts here
  if(!WIFIreceive("354")){WIFIclose;return 0;}    // 354 means server is ready to receive content
  WIFIsend(SUBJECTLINE "\r\n\r\n");      //subject line
  ltoa(coretemp,num,10);    // convert number to string
  DEBUG.println(num);
  WIFIsend("\r\nCore temperature is ");      //general message
  WIFIsend(num);      //temperature
  WIFIsend(" degrees.\r\n\r\n.\r\n");      //end of email is indicated by \r\n.\r\n
  if(!WIFIreceive("250")){WIFIclose;return 0;}    //    250 here means mail successfully sent
  WIFIsend("QUIT\r\n");      //close connection
  if(!WIFIreceive("221")){WIFIclose;return -1;}    //  221 means connection closed cleanly, so fail here is not a problem as long as email was sent
  WIFIclose();
  return 1;
}

void WIFIclose(){
  DEBUG.print("DISC!:");                                  //smtp to be closed from client side
  DEBUG.println(WIFIcmd("AT+CIPCLOSE","OK\r\n",2000));  
}

int WIFIreceive(char* s){     //wait for received ipd data to match string s
  int k=0;
  long t;
  t=millis();
  while(millis()-t<2000){
    if(WIFI.find(s)){
      t=millis()-2001;
      k=1;
    }
  }
  return k;
}

int WIFIsend(char* s){  //send string s in single connection mode
  long t;
  WIFI.print(F("AT+CIPSEND="));
  WIFI.println(strlen(s));      //data has length
  t=millis();
  while(millis()-t<1000){
    while(WIFI.available()){
      if(WIFI.find(">")){t=millis()-1001;}   //we've got the prompt
    }    
  }
  WIFI.print(s);
  t=millis();
  while(millis()-t<1000){
    while(WIFI.available()){
      if(WIFI.find("SEND OK")){t=millis()-1001;}   //send ok, timeout t
    }    
  }
  delay(100);     //still seems to need a delay, especially with consecutive sends
}

int WIFIcmd(char* c,char* r,long tmout){   //command c (nocrlf needed), returns true if response r received, otherwise times out
  long t;
  WIFI.println(c);
  t=millis();
  while(millis()-t<tmout){
    while(WIFI.available()){
      if(WIFI.find(r)){return 1;}   //response good
    }    
  }
  return 0;       //response not found
}

void wifipurge(){
  while(WIFI.available()){
    WIFI.read();
    }
}

void wifiinit(){
  DEBUG.print(F("Reset:"));
  DEBUG.println(WIFIcmd("AT+RST","ready\r\n",5000));
  DEBUG.print("QAP:");
  DEBUG.println(WIFIcmd("AT+CWQAP","OK\r\n",5000));
  DEBUG.print("AP:");
  DEBUG.println(WIFIcmd("AT+CWJAP=\""  SSIDNAME  "\",\"" SSIDPWD  "\"","WIFI GOT IP\r\n",10000));
  delay(2000);
  DEBUG.print("IP data:");
  long t;
  WIFI.println("AT+CIFSR");
  t=millis();
  while(t+1000>millis()){
    if(WIFI.available()) {
      int inByte = WIFI.read();
      DEBUG.write(inByte);
    }
  }
  DEBUG.print(F("Echo:"));
  DEBUG.println(WIFIcmd("ATE0","OK\r\n",1000));
  DEBUG.print(F("Mode:"));
  DEBUG.println(WIFIcmd("AT+CWMODE=1","OK\r\n",2000));
}

