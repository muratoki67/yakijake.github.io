#include <LiquidCrystal_I2C.h>

// 各種ピン設定
#define trigapin 8  // 超音波センサーのトリガーピン（信号を送るピン）
#define ekopin 9    // 超音波センサーのエコーピン（反射音波を受け取るピン）
#define buzzerPin 10 // ブザーを接続するピン（距離測定後にブザーを鳴らすため）
#define touchPin 7   // タッチセンサーを接続するピン（スタートボタンとして使用）

// 測定に使用する変数
float keisokujikan = 0;  // 超音波が往復するのにかかった時間（秒単位）
float kyori = 0;         // 測定された距離（センチメートル単位）

// LCDディスプレイ設定（I2C接続、20x4のディスプレイ）
LiquidCrystal_I2C lcd(0x27, 20, 4);

// ストップウォッチ用の変数
unsigned long startTime;    // 計測開始時間
unsigned long lapStartTime; // 各ラップの開始時間
bool running = false;       // ストップウォッチが動作中かどうか
bool paused = false;        // 一時停止中かどうか
bool finished = false;      // 計測終了フラグ（終了時に使用）
bool startPressed = false;  // スタートボタンが押されたかどうか
const int totalLaps = 10;   // 計測するラップ数（今回は10ラップ）
float lapTimes[totalLaps];  // 各ラップの記録
int currentLap = 0;         // 現在のラップ数

// 初期設定
void setup() {
  Serial.begin(9600);  // シリアル通信の開始（デバッグ用）
  
  // 各ピンの設定
  pinMode(ekopin, INPUT);         // エコーピンは入力ピン（信号受け取り）
  pinMode(trigapin, OUTPUT);      // トリガーピンは出力ピン（信号送信）
  pinMode(buzzerPin, OUTPUT);     // ブザーは出力ピン
  pinMode(touchPin, INPUT_PULLUP); // タッチセンサーは入力プルアップ設定

  // LCDの初期設定
  lcd.init();                   // LCDの初期化
  lcd.backlight();              // バックライトをオン
  lcd.clear();                  // LCD画面をクリア
  lcd.setCursor(0, 0);          // カーソルを1行目の先頭に設定
  lcd.print("Press the start"); // スタートボタンを押すように表示
  lcd.setCursor(0, 1);          // カーソルを2行目の先頭に設定
  lcd.print("button to begin"); // スタートボタンを押すように表示
}

// 超音波センサーによる距離測定
float tyoonpasokutei() {
  digitalWrite(trigapin, LOW);  // トリガーピンをLOWにして、前回の信号をリセット
  delayMicroseconds(2);         // 2マイクロ秒待機して、確実にLOWの状態にする
  
  digitalWrite(trigapin, HIGH);  // トリガーピンをHIGHにして、超音波信号を送信
  delayMicroseconds(10);         // 10マイクロ秒間信号を送る
  digitalWrite(trigapin, LOW);   // トリガーピンを再度LOWに設定し、信号送信を終了

  keisokujikan = pulseIn(ekopin, HIGH) / 2;  // エコーピンで反射音波を受信し、往復時間を計測。往復時間を半分にする
  kyori = keisokujikan * 340 * 100 / 1000000; // 音速（340m/s）を使って距離を算出（センチメートル単位）
  return kyori;  // 測定した距離を返す
}

void loop() {
  if (!startPressed) {  // スタートボタンが押されるまで待機
    if (digitalRead(touchPin) == LOW) {  // タッチセンサーが押されたら
      startPressed = true;  // スタートボタンが押されたフラグを立てる
      lcd.clear();  // LCD画面をクリア
    }
    return;  // スタートボタンが押されるまで処理を抜ける
  }

  if (!running && !finished) {  // 計測が開始されていない、かつ計測が終了していない場合
    startTime = millis();  // 計測開始時間を現在時刻で設定
    lapStartTime = startTime;  // 最初のラップ開始時刻も設定
    running = true;  // ストップウォッチを開始
    paused = false;  // 一時停止状態を解除
    currentLap = 0;   // ラップカウントをリセット
  }

  if (running && currentLap < totalLaps) {  // 計測中で、かつまだラップ数が指定の数より少ない場合
    float sokuteikyori = tyoonpasokutei();  // 距離を測定

    // 測定された距離が2cm以上、10cm以下の場合にラップ計測を行う
    if (sokuteikyori <= 10 && sokuteikyori > 2) {
      if (!paused) {  // 一時停止中でなければ
        paused = true;  // 一時停止状態にする
        unsigned long currentTime = millis();  // 現在時刻を取得
        lapTimes[currentLap] = (currentTime - lapStartTime) / 1000.0;  // ラップタイムを秒単位で記録
        lapStartTime = currentTime;  // 新しいラップの開始時刻を設定
        digitalWrite(buzzerPin, HIGH);  // ブザーを鳴らす

        Serial.print("Lap ");
        Serial.print(currentLap + 1);  // ラップ番号を表示
        Serial.print(": ");
        Serial.print(lapTimes[currentLap], 3);  // ラップタイムを表示（小数点以下3桁）
        Serial.println(" seconds");

        lcd.clear();  // LCD画面をクリア
        lcd.setCursor(0, 0);  // カーソルを1行目に設定
        lcd.print("Lap ");
        lcd.print(currentLap + 1);  // ラップ番号を表示
        lcd.setCursor(0, 1);  // カーソルを2行目に設定
        lcd.print("Time: ");
        lcd.print(lapTimes[currentLap], 3);  // ラップタイムを表示（小数点以下3桁）
        lcd.print(" sec");

        currentLap++;  // 次のラップに進む
      }
    } else {  // 距離が10cmを超えていたり、2cm未満だった場合
      if (paused) {  // 一時停止中であれば
        paused = false;  // 一時停止状態を解除
        digitalWrite(buzzerPin, LOW);  // ブザーを止める
      }
    }

    // 一時停止中でなければ、現在の経過時間をLCDに表示
    if (!paused) {
      unsigned long elapsedTime = millis() - lapStartTime;  // 経過時間を計算
      lcd.clear();  // LCD画面をクリア
      lcd.setCursor(0, 0);  // カーソルを1行目に設定
      lcd.print("Lap ");
      lcd.print(currentLap + 1);  // 現在のラップ番号を表示
      lcd.setCursor(0, 1);  // カーソルを2行目に設定
      lcd.print("Time: ");
      lcd.print(elapsedTime / 1000.0, 3);  // 経過時間を表示（小数点以下3桁）
      lcd.print(" sec");
    }
  } else if (running && currentLap >= totalLaps) {  // すべてのラップが終了した場合
    float totalTime = 0;  // 合計時間を初期化
    for (int i = 0; i < totalLaps; i++) {  // 各ラップのタイムを合計
      totalTime += lapTimes[i];
    }
    float averageLapTime = totalTime / totalLaps;  // 平均ラップタイムを計算

    Serial.print("Average Lap Time: ");
    Serial.print(averageLapTime, 3);  // 平均ラップタイムを表示（小数点以下3桁）
    Serial.println(" seconds");

    lcd.clear();  // LCD画面をクリア
    lcd.setCursor(0, 0);  // カーソルを1行目に設定
    lcd.print("Avg Lap Time: ");
    lcd.setCursor(0, 1);  // カーソルを2行目に設定
    lcd.print(averageLapTime, 3);  // 平均ラップタイムを表示（小数点以下3桁）
    lcd.print(" sec");

    running = false;  // 計測終了
    finished = true;  // 計測完了フラグを立てる
    digitalWrite(buzzerPin, LOW);  // ブザーをオフ
  }

  delay(100);  // 次の計測まで100ミリ秒の遅延
}
