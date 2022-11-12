# discord-worm

Simple program that fetches all tokens and sends message to all friends and servers

## Building

```bash
git clone --recurse-submodules https://github.com/birds1338/discord-worm.git
mkdir discord-worm/build && cd discord-worm/build
cmake .. -G "Visual Studio 16 2019" -A x64
# At this stage you should edit the defines at the top of main.cpp
cmake --build . --config MinSizeRel
```

## Virus scan
![Virus Scan](https://antiscan.me/images/result/Yxt6ODU0iGWO.png)

## ⚠️ Disclaimer
Don't use this malicously, only for educational purposes