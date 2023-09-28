# Renesas RX72N EnvisionKit with TOPPERS OS


本デモはルネサス <b> RX72N EnvisionKit </b> に<b>Toopers RTOS </b> を適用し<b>WolfSSL</b>の動作を確認します


## 本デモについては以下が必要です


1.Renesas e² studio Version: 2022-07 (22.7.0)以降

2.Renesas e² studio BSP

3.Toppers OS 1.9.1 (Patch RX720N適用版)


|要素|名称/バージョン|
|:--|:--|
|Renesas e² studio Version|GR-2022-07 (22.7.0)以降|
|Toppers OS|1.9.3|
|Toppers コンフィギュレータ|1.9.6|
|Renesas BSP r_bsp|7.10|
|Renesas BSP r_cmt_rx|5.10|


上記バージョンの指定については、下記に示す通り指定バージョンに修正を加える為、使用バージョンを限定しています

## 事前準備

`wolfssl-examples/TOPPERS`を`wolfssl/IDE/Renesas/e2studio/RX72N/`へコピーします
# 以下に環境構築手順を示します
## 1.wolfSSLライブラリーのビルド
本デモに必要なToppersライブラリー、wolfSSLライブラリーを作成します

1-1.　メニュー[ファイル]->[ファイルシステムからプロジェクトを開く]をクリック

1-2.　[プロジェクトをインポート]ダイアログの[ルートディレクトリーの選択(T)]の[参照(R)]を押下

1-3.　git レポジトリwolfssl/IDE/Renesas/e2studio/RX72N/Toppersの`wolflib`を選択[フォルダーの選択]を押下

1-4.　プロジェクト・エクスプローラーの作成したプロジェクトをクリック後プルダウンメニューから[プロジェクトのビルド(B)]キーを選択しビルド

(上記操作操作を行う場合[プロジェクト][プロパティ]の[設定][toolchain]タブで[ツールチェーン:]が選択されている事を確認ください)

1-5. 　`wolflib/Debug`に`libwolflib.a`が生成されます

## 2. TOPPERSライブラリーのビルド

2-1.　Toppersライブラリーのビルドの為、[asp-1.9.3 Renesas BSP 適用version](https://www.toppers.jp/asp-d-download.html)をダウンロードし`/wolfssl/IDE/Renesas/e2studio/RX72N/Toppers/`に解凍します

2-2.　Toppersライブラリーのビルドの為、[コンフィギュレータ Release 1.9.6（Windows用バイナリ）]([https://www.toppers.jp/cfg-download.html] )をダウンロードし 2.1 で解凍した `asp` ディレクトリに `cfg/cfg` ディレクトリを作成し中に `cfg.exe` として解凍します

2-3.　Patch適用の為、以下に示すShellスクリプトを実行します \
   (EDMAC使用に必要なファイルをコピーします)


 ```
 $ pwd
[個別インストール環境]/wolfssl/IDE/Renesas/e2studio/RX72N/Toppers
 ./setting.sh
```

2-4.事前準備確認
   コマンド実行ではMsys2等の環境を事前にご用意ください。Msys2でgccのツールチェーンのインストールを行ってください
   Msys2環境では事前にRenesas環境のパス設定をbashrc等で行う必要があります.
---設定例を示します：ルネサスツールチェーンパスを指定---

 ```
export PATH=PATH=$PATH:\/C/ProgramData\/GCC\ for\ Renesas\ RX\ 8.3.0.202202-GNURX-ELF/rx-elf/rx-elf/bin
 ```

2-5.設定を確認後、以下を行います

  ```
$ pwd
[個別インストール環境]/wolfssl/IDE/Renesas/e2studio/RX72N/Toppers/asp
$ perl ./configure -T rx72n_gcc
$ make depend
```


2-7.　メニューの[ファイル・システムからプロジェクトを開く...]を選択

2-8.　git レポジトリwolfssl/IDE/Renesas/e2studio/RX72N/Toppersの`Toppers_app`を選択[フォルダーの選択]を押下

2-6.　プロジェクト[プロパティ]->[C/C++ビルド]→[環境]ダイアログ[設定する環境変数]の[追加]ボタンを押下、[新規変数]ダイアログの[名前:]に[C_PROJECT]を入力、[値:]に[${ProjDirPath}]を入力します。

2-9.　プロジェクト・エクスプローラーの作成したプロジェクトをクリック後プルダウンメニューから[プロジェクトのビルド(B)]キーを選択しビルド

2-10.　`toppers_rx`に`libasp.a`が生成されます

## 3. wolfSSLDemoプロジェクトのビルド
 3-1. メニューの[ファイル・システムからプロジェクトを開く...]を選択

 3-2. git レポジトリwolfssl/IDE/Renesas/e2studio/RX72N/Toppersの[wolfSSLDemo]を選択[フォルダーの選択]を押下

 3-3. [WolfSSLDemo.scfg]をダブルクリックで設定ダイアログが表示→[コンポーネントタブ] を選択
 3.4. [ソフトウェアコンポーネントダイアログ]ダイアログ右上の[コードの生成]を押下
 3.5. ダイアログ左のコンポーネント選択で[Startup] [r_bsp]を選択右クリックしコンテキストメニュー[バージョン変更]を選択し[現在のバージョン]が[7.10]である事を確認してください([7.10]でない場合[変更後のバージョン:]で[7.10]を選択し[次へ(N)>]を押下しコードを生成して下さい)

 3-6. ダイアログ左のコンポーネント選択で[Drivers] [r_cmt_rx]を選択右クリックしコンテキストメニュー[バージョン変更]を選択し[現在のバージョン] が[5.10]である事を確認してください([5.10]でない場合[変更後のバージョン:]で[5.10]を選択し[次へ(N)>]を押下しコードを生成して下さい)

 3-7.生成されたBSPをToppersに適用する為、Patch コマンドにて修正をします
 (Msys2でpatchコマンドが使えない場合は[pacman -S patch] でインストールが必要となります)
 以下を行います

 ```
$ pwd
[個別インストール環境]/wolfssl/IDE/Renesas/e2studio/RX72N/Toppers/WolfSSLDemo
 patch --binary -p0 < ./bsp.patch
```

Note:
スマートコンフィギュレーターを使用して[r_bsp],[r_cmt_rx]のコードを生成した場合、パッチを再適用する必要があります。


 3-8.[3-1.]終了後プルダウンメニューから[プロジェクトのビルド(B)]キーを選択しビルド

 3-9.ビルドで生成されたELFファイルを[メニュー]→[実行(R)]→[実行(R)]又は[デバッグ(D)]でボードへ転送を行い、実行します。

 注:コンフィグレーション直後/ビルドクリヤー後に[T4_Library_ether_ccrx_rxv1_little]がリンカーでエラーになる場合が
 ありますがプロジェクトの[プロパティ]ダイアログ[C/C++ビルド]の[設定]で[Linker]/[Archives]/[User defined archive (library) files (-I)]/[×]押下から[T4_Library_ether_ccrx_rxv1_little]を削除してください

### 3-1. Server プログラムの実行
 3-1-1. `wolf_demo.h`のdefine値 `#define WOLFSSL_SERVER_TEST` を定義を行うとサーバとしての動作になります。DHCPを使ってIPアドレスを取得します。

 3-1-2. `Renesas Debug Virtual Console` にて実行を確認します

```
Start WolfSSL Demo !!
Accept DHCP.ipaddr[4]   192.168.11.6
Accept DHCP.maskaddr[4] 255.255.255.0
Accept DHCP.gwaddr[4]   192.168.11.1
Accept DHCP.dnsaddr[4]  192.168.11.1
Accept DHCP.dnsaddr2[4] 0.0.0.0
Accept DHCP.macaddr[6]  74:90:50:10:FE:77
Accept DHCP.domain[0]

Start TLS Server
```

 3-1-3. サーバが取得したIPアドレスに対して対抗のクライアントを実行します。
 ```
 $ ./examples/client/client -h 192.168.11.6 -p 11111
SSL version is TLSv1.2
SSL cipher suite is TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384
SSL curve name is SECP256R1
hello wolfssl!
 ```

サーバ側に以下のような出力があればTLS通信を行っています。

```
Start TLS Server
SSL Accept
SSL Read
Received Data: hello wolfssl!
my_IOSend NG
Start TLS Server

```

### 3-2. Client プログラムの実行
 3-2-1. `wolf_demo.h`のdefine値 `#define WOLFSSL_CLIENT_TEST` の定義を有効にします。

 3-2-2. `wolf_demo.h` のサーバIPアドレスのdefine値 `SERVER_IP`を "xxx.xx.xx.xx" ポート番号のdefine値 `SERVER_PortNo`をポート番号として設定して下さい

 3-2-3. 対抗のサーバを起動します

 ```
$ ./examples/server/server -b -d -i -v 4
SSL version is TLSv1.3
SSL cipher suite is TLS_AES_128_GCM_SHA256
SSL curve name is SECP256R1
Client message: Hello Server

SSL version is TLSv1.3
SSL cipher suite is TLS_AES_128_GCM_SHA256
SSL curve name is SECP256R1
Client message: Hello Server

 ```

 3-2-3. `Renesas Debug Virtual Console` にて実行を確認します。実行を停止するまでTLS接続を繰り返します。
 ```
 Start WolfSSL Demo !!
Accept DHCP.ipaddr[4]   192.168.11.6
Accept DHCP.maskaddr[4] 255.255.255.0
Accept DHCP.gwaddr[4]   192.168.11.1
Accept DHCP.dnsaddr[4]  192.168.11.1
Accept DHCP.dnsaddr2[4] 0.0.0.0
Accept DHCP.macaddr[6]  74:90:50:10:FE:77
Accept DHCP.domain[0]

Start TLS Client
Received: I hear you fa shizzle!
Start TLS Client
Received: I hear you fa shizzle!
Start TLS Client
Received: I hear you fa shizzle!

 ```
### 3-3. Cryptテストの実行
 3-3-1. `wolf_demo.h`のdefine値 `#define WOLFCRYPT_TEST` の定義を有効にします。

 3-3-2. `Renesas Debug Virtual Console` にて実行を確認します。

 ```
 Start WolfSSL Demo !!


 Start wolfCrypt Test
------------------------------------------------------------------------------
 wolfSSL version 5.6.3
------------------------------------------------------------------------------
error    test passed!
MEMORY   test passed!
base64   test passed!
asn      test passed!
RANDOM   test passed!
MD5      test passed!
MD4      test passed!
SHA      test passed!
SHA-256  test passed!
SHA-384  test passed!
SHA-512  test passed!
SHA-512/224  test passed!
SHA-512/256  test passed!
Hash     test passed!
HMAC-MD5 test passed!
HMAC-SHA test passed!
HMAC-SHA256 test passed!
HMAC-SHA384 test passed!
HMAC-SHA512 test passed!
HMAC-KDF    test passed!
TLSv1.3 KDF test passed!
GMAC     test passed!
DES      test passed!
DES3     test passed!
AES      test passed!
AES192   test passed!
AES256   test passed!
AES-GCM  test passed!
RSA      test passed!
DH       test passed!
DSA      test passed!
PWDBASED test passed!
ECC      test passed!
ECC buffer test passed!
CURVE25519 test passed!
ED25519  test passed!
logging  test passed!
time test passed!
mutex    test passed!
memcb    test passed!
Test complete
 End wolfCrypt Test
 ```

### 3-3. ベンチマークテストの実行
 3-3-1. `wolf_demo.h`のdefine値 `#define WOLF_BENCHMARK` の定義を有効にします。

 3-3-2. `Renesas Debug Virtual Console` にて実行を確認します。

 ```
Start WolfSSL Demo !!
 Start wolfCrypt Benchmark
wolfCrypt Benchmark (block bytes 1024, min 1.0 sec each)
RNG                        275 KiB took 1.036 seconds,  265.444 KiB/s
AES-128-CBC-enc            725 KiB took 1.017 seconds,  712.671 KiB/s
AES-128-CBC-dec            700 KiB took 1.025 seconds,  682.727 KiB/s
AES-192-CBC-enc            675 KiB took 1.006 seconds,  671.041 KiB/s
AES-192-CBC-dec            650 KiB took 1.009 seconds,  644.266 KiB/s
AES-256-CBC-enc            650 KiB took 1.024 seconds,  634.518 KiB/s
AES-256-CBC-dec            625 KiB took 1.023 seconds,  610.709 KiB/s
AES-128-GCM-enc            150 KiB took 1.040 seconds,  144.175 KiB/s
AES-128-GCM-dec            150 KiB took 1.041 seconds,  144.134 KiB/s
AES-192-GCM-enc            150 KiB took 1.054 seconds,  142.356 KiB/s
AES-192-GCM-dec            150 KiB took 1.054 seconds,  142.315 KiB/s
AES-256-GCM-enc            150 KiB took 1.067 seconds,  140.607 KiB/s
AES-256-GCM-dec            150 KiB took 1.067 seconds,  140.581 KiB/s
AES-128-GCM-enc-no_AAD     150 KiB took 1.028 seconds,  145.929 KiB/s
AES-128-GCM-dec-no_AAD     150 KiB took 1.028 seconds,  145.886 KiB/s
AES-192-GCM-enc-no_AAD     150 KiB took 1.041 seconds,  144.065 KiB/s
AES-192-GCM-dec-no_AAD     150 KiB took 1.041 seconds,  144.023 KiB/s
AES-256-GCM-enc-no_AAD     150 KiB took 1.054 seconds,  142.288 KiB/s
AES-256-GCM-dec-no_AAD     150 KiB took 1.054 seconds,  142.248 KiB/s
GMAC Default               184 KiB took 1.000 seconds,  183.963 KiB/s
3DES                       275 KiB took 1.016 seconds,  270.536 KiB/s
MD5                          5 MiB took 1.002 seconds,    4.900 MiB/s
SHA                          2 MiB took 1.013 seconds,    1.857 MiB/s
SHA-256                    625 KiB took 1.004 seconds,  622.634 KiB/s
SHA-384                    475 KiB took 1.011 seconds,  469.832 KiB/s
SHA-512                    475 KiB took 1.011 seconds,  469.832 KiB/s
SHA-512/224                475 KiB took 1.011 seconds,  469.785 KiB/s
SHA-512/256                475 KiB took 1.011 seconds,  469.785 KiB/s
HMAC-MD5                     5 MiB took 1.001 seconds,    4.855 MiB/s
HMAC-SHA                     2 MiB took 1.008 seconds,    1.841 MiB/s
HMAC-SHA256                625 KiB took 1.012 seconds,  617.833 KiB/s
HMAC-SHA384                475 KiB took 1.027 seconds,  462.512 KiB/s
HMAC-SHA512                475 KiB took 1.027 seconds,  462.512 KiB/s
PBKDF2                      96 bytes took 1.232 seconds,   77.890 bytes/s
RSA     2048   public        18 ops took 1.020 sec, avg 56.639 ms, 17.656 ops/sec
RSA     2048  private         2 ops took 6.787 sec, avg 3393.650 ms, 0.295 ops/sec
DH      2048  key gen         2 ops took 1.466 sec, avg 732.950 ms, 1.364 ops/sec
DH      2048    agree         2 ops took 3.182 sec, avg 1590.800 ms, 0.629 ops/sec
ECC   [      SECP256R1]   256  key gen         4 ops took 1.704 sec, avg 425.875 ms, 2.348 ops/sec
ECDHE [      SECP256R1]   256    agree         4 ops took 1.700 sec, avg 425.050 ms, 2.353 ops/sec
ECDSA [      SECP256R1]   256     sign         4 ops took 1.716 sec, avg 429.050 ms, 2.331 ops/sec
ECDSA [      SECP256R1]   256   verify         2 ops took 1.642 sec, avg 821.100 ms, 1.218 ops/sec
CURVE  25519  key gen         2 ops took 1.343 sec, avg 671.700 ms, 1.489 ops/sec
CURVE  25519    agree         2 ops took 1.342 sec, avg 670.900 ms, 1.491 ops/sec
ED     25519  key gen        69 ops took 1.009 sec, avg 14.617 ms, 68.412 ops/sec
ED     25519     sign        60 ops took 1.022 sec, avg 17.038 ms, 58.691 ops/sec
ED     25519   verify        22 ops took 1.030 sec, avg 46.836 ms, 21.351 ops/sec
Benchmark complete
 End wolfCrypt Benchmark
 ```

