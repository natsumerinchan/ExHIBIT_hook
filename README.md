# ExHIBIT引擎 hook补丁

## 实现功能

- 1、优先从rld_chs读取脚本
- 2、字体修改
- 3、日志系统
- 4、标题修改
- 5、编码修改(0x80为cp932编码,0x86为GBK编码)

## 使用方法

理论上适用于所有未加壳的ExHIBIT引擎游戏。  
安装`Visual Studio 2026`后打开本项目，选择`x86-Release`并生成`ExHIBIT_hook.dll`。  
最后使用`tools\setdll.exe /d:ExHIBIT_hook.dll .\ExHIBIT_CHS.exe`导入dll。  
（setdll.exe来自Detours项目）

## 已测试游戏

- [【La'cryma】空を飛ぶ、7つ目の魔法。](https://vndb.org/v1617)
- [【SkyFish】蒼穹のソレイユ～FULLMETAL EYES～](https://vndb.org/v4910)
- [【SkyFish poco】にゃんカフェマキアート ～猫がいるカフェのえっち事情～](https://vndb.org/v12505)
- [【MOONSTONE】Magical Marriage Lunatics!!](https://vndb.org/v12559)
- [【MOONSTONE】マジスキ～Marginal Skip～](https://vndb.org/v1140)
- [【POISONエクスタシー】らぶらぶ♥プリンセス ～お姫さまがいっぱい！もっとエッチなハーレム生活!!～](https://vndb.org/v16657)
- [【POISONエクスタシー】らぶ♥らぶ♥らいふ ～お嬢様７人とラブラブハーレム生活～](https://vndb.org/v13842)

