# Qt: MP3Tag
一个基于 Qt 的 Mp3 文件 ID3信息读取程序。  

![alt](preview.png)

## [MP3文件结构](https://blog.csdn.net/fulinwsuafcie/article/details/8972346)
<table>
<tr><td>ID3V2</td></tr>
<tr><td>Frame<br>.<br>.<br>.<br>Frame</td></tr>
<tr><td>ID3V1</td></tr>
</table>

## ID3V1
位于MP3文件末尾的128字节。  
<table>
<tr><td>长度(字节)</td><td>说明</td></tr>
<tr><td>3</td><td>"TAG"标识符(不含则没有ID3V1)</td></tr>
<tr><td>30</td><td>歌名</td></tr>
<tr><td>30</td><td>歌手</td></tr>
<tr><td>30</td><td>专辑</td></tr>
<tr><td>4</td><td>年份</td></tr>
<tr><td>28</td><td>备注</td></tr>
<tr><td>1</td><td>保留</td></tr>
<tr><td>1</td><td>音轨</td></tr>
<tr><td>1</td><td>类别</td></tr>
</table>

信息不足长度的用 00 填充。

## ID3V2
ID3V2 位置在 MP3 文件的开头，长度不定，由一个标签头和若干个标签帧或一个扩展标签头组成。  
### 1.标签头
<table>
<tr><td>长度(字节)</td><td>说明</td></tr>
<tr><td>3</td><td>"ID3"标识符(不含则没有ID3V2)</td></tr>
<tr><td>1</td><td>版本号，ID3V2.3 就记录3</td></tr>
<tr><td>1</td><td>副版本号</td></tr>
<tr><td>1</td><td>标志</td></tr>
<tr><td>4</td><td>标签大小，包括标签头10 个字节和所有标签帧的大小</td></tr>
</table>

#### 1)标签大小计算规则
4 个字节 32 位，但是每个字节只用 7 位，最高位不使用，计算大小时要将最高位去掉，得到一个 28 位的二进制数（坑爹）。

### 2.标签帧
由10 个字节的帧头和至少一个字节的不固定长度的内容组成。  

#### 1)帧头  
<table>
<tr><td>长度(字节)</td><td>说明</td></tr>
<tr><td>4</td><td>帧标识</td></tr>
<tr><td>4</td><td>帧内容大小</td></tr>
<tr><td>2</td><td>帧标志</td></tr>
</table>

##### 1>帧标识

<table>
<tr><td>TIT2</td><td>标题</td></tr>
<tr><td>TPE1</td><td>歌手</td></tr>
<tr><td>TALB</td><td>专辑</td></tr>
<tr><td>TRCK</td><td>音轨格式：N/M 其中N 为专辑 中的第N 首，M为专辑 中共M 首，N和M 为ASCII 码表示的数字</td></tr>
<tr><td>TYER</td><td>年代，用 ASCII 码表示的数字(00开头 00结尾) </td></tr>
<tr><td>TCON</td><td>类型，直接用字符串表示</td></tr>
<tr><td>COMM</td><td>备注，格式："eng FF FE 00 00 备注内容"，其中 eng 表示备注所使用的自然语言</td></tr>
</table>

##### 2> 帧大小
只表示帧内容的大小，不包含帧头的10个字节。  

#### 2)帧内容
一个字节来说明这个帧是使用 ISO-8859-1 编码还是使用 Unicode 编码的。如果是使用 ISO-8859-1 则这个字节为0，如果是使用 Unicode 则这个字节为1，使用 Unicode Bom (0xFF 0xFE) 开头和两个 0 (0x00 0x00) 作为结尾。

### [ID3乱码](http://tieba.baidu.com/p/5410932979)  
ID3v1：只支持 ISO-8859-1  
ID3v2.3：ISO-8859-1、UTF-16  
ID3v2.4：ISO-8859-1、UTF-16、UTF-8  
APEv2：UTF-8  
ISO-8859-1 别名 Latin1。  
ID3v1中保存的字符串多采用ANSI编码。因此根据系统语言的不同所使用的编码是不同的。如果是简体中文系统使用的是GB2312，繁体中文就是BIG5，日文操作系统下面又是JIS编码。所以，如果MP3中仅有ID3v1的信息，那在不同语言的操作系统中的播放器里面显示的信息会是乱码。  
所以如果要保存中文信息，最好使用 ID3v2.3 保存 Unicode 字符串，这样MP3在各个系统下面播放就不会出现乱码的问题了。