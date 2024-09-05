# カレンダー

C言語とSQLiteで予定入力できるカレンダー

追加パッケージのインストール
```
sudo apt install sqlite3
```

gccでコンパイルするときに、-lsqlite3 オプションを追加してSQLiteライブラリをリンクします。
```
gcc calendar.c -o calendar -lsqlite3
```

実行するとカレンダーが表示されます。
```
./calendar

現在の年月日: 2024年 9月 5日

     Sep 2024

 Su Mo Tu We Th Fr Sa
  1  2  3  4  5  6  7
  8  9 10 11 12 13 14
 15 16 17 18 19 20 21
 22 23 24 25 26 27 28
 29 30

次の月(n)、前の月(p)、予定追加(a)、予定確認(c)、年月指定(s)、終了(q):
```

