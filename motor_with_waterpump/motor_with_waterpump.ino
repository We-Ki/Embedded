int Dir1Pin_A = 7;      // 제어신호 1핀
int Dir2Pin_A = 8;      // 제어신호 2핀
int SpeedPin_A = 6;    // PWM제어를 위한 핀
 
void setup() {
  pinMode(Dir1Pin_A, OUTPUT);             // 제어 1번핀 출력모드 설정
  pinMode(Dir2Pin_A, OUTPUT);             // 제어 2번핀 출력모드 설정
  pinMode(SpeedPin_A, OUTPUT);            // PWM제어핀 출력모드 설정
}
 
void loop() {
 
  digitalWrite(Dir1Pin_A, HIGH);         //모터가 시계 방향으로 회전
  digitalWrite(Dir2Pin_A, LOW);
  analogWrite(SpeedPin_A, 45);          //모터 속도를 45로 설정
  delay(5000);
 
  digitalWrite(Dir1Pin_A, HIGH);         //모터가 시계 방향으로 회전
  digitalWrite(Dir2Pin_A, LOW);
  analogWrite(SpeedPin_A, 0);           //모터 속도를 1/5로 설정
  delay(3000);
 
}
