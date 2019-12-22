# LED-control-driver-for-optical-Morse-communication
ロボットシステム学の課題としてGPIO25番を操作し光モールス通信を行うGNU/Linux用デバイスドライバを製作した。
和文モールスや送信開始等々の略符号及び非ASCII文字には非対応。つまり英文および数字と多少の記号のみ送信可能。

## 使用方法  
$ echo [英文] > /dev/myled0  
$ cat [英文が書かれたファイル] > /dev/myled0  
といった風に使う

参考
https://ja.wikipedia.org/wiki/モールス符号
