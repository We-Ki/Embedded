int Dir1Pin_A = 10;      // 제어신호 1핀 3.3V and 5V에서 정상작동
int Dir2Pin_A = 13;      // 제어신호 2핀
int SpeedPin_A =9;    // PWM제어를 위한 핀

int Dir1Pin_B = 27;      // 제어신호 1핀
int Dir2Pin_B = 26;      // 제어신호 2핀
int SpeedPin_B =25;   // PWM제어를 위한 핀
 
void setup() {
  pinMode(Dir1Pin_A, OUTPUT);             // 제어 1번핀 출력모드 설정
  pinMode(Dir2Pin_A, OUTPUT);             // 제어 2번핀 출력모드 설정
  pinMode(SpeedPin_A, OUTPUT);            // PWM제어핀 출력모드 설정

  pinMode(Dir1Pin_B, OUTPUT);             // 제어 1번핀 출력모드 설정
  pinMode(Dir2Pin_B, OUTPUT);             // 제어 2번핀 출력모드 설정
  pinMode(SpeedPin_B, OUTPUT);            // PWM제어핀 출력모드 설정
}
 
void loop() {
 
  digitalWrite(Dir1Pin_A, HIGH);         //모터가 시계 방향으로 회전
  digitalWrite(Dir2Pin_A, LOW);
  analogWrite(SpeedPin_A, 70);
  digitalWrite(Dir1Pin_B, LOW);         //워터펌프가 시계 방향으로 회전
  digitalWrite(Dir2Pin_B, HIGH);
  analogWrite(SpeedPin_B, 255);           //모터 속도를 45로 설정
  delay(5000);
 
  digitalWrite(Dir1Pin_A, HIGH);         //모터가 시계 방향으로 회전
  digitalWrite(Dir2Pin_A, LOW);
  analogWrite(SpeedPin_A, 0); 
  digitalWrite(Dir1Pin_B, LOW);         //워터펌프가 시계 방향으로 회전
  digitalWrite(Dir2Pin_B, HIGH);
  analogWrite(SpeedPin_B, 0);           
  delay(3000);
 

 
}
