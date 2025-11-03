# ENV IV Sensor - M5Stack Core2 環境センサー表示

## 概要

このプロジェクトは、M5Stack Core2とENV IV Unitを使用して、温度・湿度・気圧を測定し、リアルタイムで画面に表示するシステムです。WiFi接続とNTPによる時刻同期機能も備えています。  
また、最新のライブラリであるM5Unifiedで記述しています。

## 主な機能

### 1. **環境センサーデータの取得**
- **SHT40**: 温度・湿度センサー
  - 温度測定範囲: -40°C〜125°C
  - 湿度測定範囲: 0〜100%
- **BMP280**: 気圧センサー
  - 気圧測定範囲: 300〜1100 hPa
  - オーバーサンプリング設定（精度向上）

### 2. **WiFi接続**
- 自動WiFi接続（保存済みの認証情報を使用）
- 接続状態の監視と表示
- リトライ機能（最大20回）

### 3. **NTP時刻同期**
- NTPサーバー: `ntp.nict.jp`（日本標準時プロジェクト）
- タイムゾーン: JST (UTC+9)
- 自動再同期: 1時間ごと

### 4. **ディスプレイ表示**
- **センサーデータ**
  - 湿度（シアン色）
  - 温度（オレンジ色）
  - 気圧（黄緑色）
- **時刻情報**
  - 日付（年/月/日、曜日）
  - 時刻（時:分:秒）
- **ステータス表示**
  - WiFi接続状態
  - NTP同期状態

### 5. **バッテリー情報**
- ボタンA押下でバッテリー情報を表示
  - バッテリー残量（%）
  - 充電状態（充電中/放電中）
- 3秒間表示後、自動的にセンサー画面に戻る

### 6. **電源管理**
- 電源ボタン長押しで安全にシャットダウン

## ハードウェア構成

### 必要なデバイス
- **M5Stack Core2**: メインコントローラー
- **ENV IV Unit**: 環境センサーユニット
  - SHT40（温度・湿度センサー）
  - BMP280（気圧センサー）

### 接続方法
- ENV IV UnitをM5Stack Core2のPort Aに接続
- I2C通信（SDA/SCL）で自動認識

## ソフトウェア構成

### 使用ライブラリ
```cpp
#include <M5Unified.h>          // M5Stack統合ライブラリ
#include <M5UnitUnified.h>      // Unitデバイス統合管理
#include <M5UnitUnifiedENV.h>   // ENV系センサー用
#include <WiFi.h>                // WiFi接続
#include "time.h"                // 時刻管理
```

### 主要な設定値

| 項目 | 値 | 説明 |
|------|-----|------|
| NTPサーバー | `ntp.nict.jp` | 日本の公式NTPサーバー |
| タイムゾーン | UTC+9 | 日本標準時 |
| NTP再同期間隔 | 3600秒 | 1時間ごとに時刻を再同期 |
| 画面更新間隔 | 500ms | 画面の描画更新頻度 |
| バッテリー表示時間 | 3000ms | 3秒間表示 |
| I2C通信速度 | 400kHz | 高速モード |

### BMP280センサー設定
```cpp
気圧オーバーサンプリング: X16   // 高精度測定
温度オーバーサンプリング: X2    // 標準精度
フィルター係数: 16              // ノイズ除去
スタンバイ時間: 500ms          // 測定間隔
```

## 動作フロー

### 起動シーケンス（setup関数）

1. **M5Stack初期化**
   - ディスプレイ設定（横向き表示）
   - 初期画面表示

2. **WiFi接続**
   - 保存済み認証情報で自動接続
   - 接続成功時にIPアドレス表示
   - 最大10秒間リトライ

3. **NTP時刻同期**
   - 時刻取得（タイムアウト10秒）
   - 同期成功/失敗を画面表示

4. **センサー初期化**
   - I2Cピン確認（SDA/SCL）
   - BMP280設定
   - ENV4 Unit追加と開始
   - エラー時は赤画面で停止

5. **描画用スプライト作成**
   - オフスクリーン描画でちらつき防止

### メインループ（loop関数）

```
┌─────────────────────────────┐
│ M5.update() / Units.update()│  ← ボタン・センサー状態更新
└──────────┬──────────────────┘
           │
           ├→ ボタンA押下？ → バッテリー表示フラグON
           │
           ├→ 電源ボタン長押し？ → シャットダウン
           │
           ├→ 1時間経過？ → NTP再同期
           │
           ├→ SHT40更新あり？ → 温度・湿度取得
           │
           ├→ BMP280更新あり？ → 気圧取得
           │
           └→ 500ms経過？ → 画面更新
                              ├→ バッテリー情報 or
                              └→ センサー情報+時刻
```

## ボタン操作

| ボタン | 操作 | 機能 |
|--------|------|------|
| ボタンA（画面左下） | 短押し | バッテリー情報表示（3秒間） |
| 電源ボタン（側面） | 長押し | 電源オフ |

## 画面レイアウト

### センサー画面（通常時）
```
=== Sensor Data ===
Humidity:    XX.X %      [シアン]
Temperature: XX.X C      [オレンジ]
Pressure:  XXXX.X hPa    [黄緑]

=== Time (NTP) ===
YYYY/MM/DD (曜日)        [黄色]
HH:MM:SS

[下部]
WiFi:OK  NTP:OK          [グレー]
```

### バッテリー画面（ボタンA押下時）
```
=== Battery Info ===

Level: XXX%              [緑]
Status: Charging/        [黄色/グレー]
        Discharging
```

## 技術的な特徴

### 1. **スプライト描画によるちらつき防止**
- オフスクリーンバッファ（`M5Canvas`）を使用
- 全ての描画をスプライトに行い、最後に一括転送
- 画面のちらつきを完全に排除

### 2. **非同期データ更新**
- センサーデータは`updated()`メソッドで新しいデータのみ処理
- 画面更新は500ms間隔で独立して実行
- 効率的なCPU使用

### 3. **条件付きコンパイル**
```cpp
#define USING_ENV4  // ENV4使用時
// または
#undef USING_ENV4   // SHT40+BMP280個別使用時
```

### 4. **エラーハンドリング**
- センサー初期化失敗時は赤画面で明示
- WiFi/NTP失敗時も継続動作
- 接続状態を常時モニタリング

## ログ出力

シリアルモニタで以下の情報を確認可能：
```
I (123) : getPin: SDA:32 SCL:33
I (456) : Units.add result: 1
I (789) : Units.begin result: 1
I (1011): M5UnitUnified has been begun
>SHT40Temp:25.3
>Humidity:45.2
>Pressure:1013.2
```

## トラブルシューティング

### センサーが認識されない
- **症状**: 赤画面で "Failed to add ENV4!" または "Failed to begin!"
- **対処**:
  - ENV IV UnitがPort Aにしっかり接続されているか確認
  - I2Cケーブルの接触不良をチェック
  - 再起動を試す

### WiFiに接続できない
- **症状**: "WiFi Failed!" 表示
- **対処**:
  - 事前にM5Burnerなどで WiFi認証情報を設定
  - WiFiルーターの電波強度を確認

### 時刻が表示されない
- **症状**: "Time not available" 表示
- **対処**:
  - WiFi接続を確認
  - NTPサーバーへのアクセスを確認
  - ファイアウォール設定を確認

## カスタマイズ方法

### NTPサーバーの変更
```cpp
const char* ntpServer = "pool.ntp.org";  // グローバルNTPプール
```

### 画面更新頻度の変更
```cpp
if (now - lastUpdate > 1000) {  // 1秒ごとに変更
```

### 色の変更
```cpp
canvas.setTextColor(GREEN);     // 好みの色に変更
// 使用可能な色: RED, GREEN, BLUE, WHITE, BLACK, 
//              CYAN, YELLOW, ORANGE, etc.
```

## 今後の予定
restapi経由でセンサーから取得した値をサーバーへ送信  
取得したデータを可視化


## ライセンス

This project is licensed under the MIT License - see the [LICENSE](./LICENSE) file for details.

## 参考リンク

- [M5Stack公式サイト](https://m5stack.com/)
- [ENV IV Unit製品ページ](https://docs.m5stack.com/en/unit/envIV)
- [M5Unified GitHub](https://github.com/m5stack/M5Unified)
- [NICT NTPサービス](https://www.nict.go.jp/JST/JST5.html)

---

**開発環境**: PlatformIO  
**対応デバイス**: M5Stack Core2  
**必須センサー**: ENV IV Unit (SHT40 + BMP280)
