# Lab4 文件系统

todo:makedep

李糖 2001210320

> 【实习建议】
>
> 1. 数据结构的修改和维护
>
> - 文件管理的升级基于对原有Nachos数据结构的修改。增加文件的描述信息需对文件头结构进行简单修改。多级目录中可创建目录也可创建文件，应根据实际的文件类型初始化文件头信息。
>
> 2. 实现多级目录应当注意
>
> - 目录文件的含义。每个目录对应一个文件，通过此文件可了解其子目录及父目录的信息。
>
> - Nachos的目录文件大小是预先定义的，但实际上，目录文件的大小应根据内容确定，且能改变。
>
> - 实现多级目录后，添加、删除目录项要根据具体的路径，对树的遍历要有深刻的理解。
>
> 3. 为了实现文件长度无限，可以采取混合索引的分配方式。

## 任务完成情况

| Exercises  | Y/N  |
| :--------: | :--: |
| Exercise1  |  Y   |
| Exercise2  |  Y   |
| Exercise3  |  Y   |
| Exercise4  |  Y   |
| Exercise5  |  Y   |
| Exercise6  |  Y   |
| Exerciseu7 |  Y   |
| Challenge1 |  Y   |
| Challenge2 |  Y   |

## 准备工作

> 我在main.cc中添加了-q(means quite)指令来关闭`verbose`变量，这样系统就不会每次都在halt时输出冗长的系统信息。为了测试文件系统，我写了一个脚本文件，主要用来测试文件系统的各种debug指令。在`terminal`中输入`my_script.sh`即可运行我放在`code/filesys/test/my_script`的脚本文件。这是脚本文件的内容：

```sh
# goto filesys/ in vagrant
cd /vagrant/nachos/nachos-3.4/code/filesys

# use -q to disable verbose machine messages
echo "=== format the DISK ==="
./nachos -q -f
echo "=== copies file \"big\" from UNIX to Nachos ==="
./nachos -q -cp test/big big
# Files: big
echo "=== copies file \"small\" from UNIX to Nachos ==="
./nachos -q -cp test/small small
# Files: big, small
echo "=== lists the contents of the Nachos directory ==="
./nachos -q -l
# big
# small
echo "=== remove the file \"big\" from Nachos ==="
./nachos -q -r big
echo "=== print the content of file \"small\" ==="
./nachos -q -p small
echo "=== prints the contents of the entire file system ==="
./nachos -q -D
echo "=== tests the performance of the Nachos file system ==="
./nachos -q -t
```

## 一、文件系统的基本操作

### Exercise 1 源代码阅读

> 阅读Nachos源代码中与文件系统相关的代码，理解Nachos文件系统的工作原理。
>
> code/filesys/filesys.h和code/filesys/filesys.cc
>
> code/filesys/filehdr.h和code/filesys/filehdr.cc
>
> code/filesys/directory.h和code/filesys/directory.cc
>
> code /filesys/openfile.h和code /filesys/openfile.cc
>
> code/userprog/bitmap.h和code/userprog/bitmap.cc

> ```
> Nachos File System
> 
> +-----------------------------------+
> |             FileSystem            |
> +-----------------------------------+
> |              OpenFile             |	 <-memory(i-node index)
> +------------+----------+-----------+
> | File Header| Directory|  Bitmap   |    <-disk(i-node)                 
> +-----------------------------------+
> |             SynchDisk             |
> +-----------------------------------+
> |                Disk               |
> +-----------------------------------+
> ```

#### dick.cc disk.h

TOBEDONE 

#### synchdisk.cc synchdisk.h

> 和其它设备一样，Nachos 模拟的磁盘是异步设备。当发出访问磁盘的请求后立刻返回，当从磁盘读出或写入数据结束后，发出磁盘中断，说明一次磁盘访问真正结束。
>
> Nachos 是一个多线程的系统，如果多个线程同时对磁盘进行访问，会引起系统的混乱。所以必须作出这样的限制： 
>
> - 同时只能有一个线程访问磁盘 
>
> - 当发出磁盘访问请求后，必须等待访问的真正结束。 这两个限制就是实现同步磁盘的目的。

```cpp
class SynchDisk
{
public:
  SynchDisk(char *name); // 生成一个同步磁盘
  ~SynchDisk();          // 析构磁盘
  void ReadSector(int sectorNumber, char *data); //同步读写磁盘，只有
  void WriteSector(int sectorNumber, char *data);//当真正读写结束才返回
  void RequestDone(); // 磁盘中断处理函数
private:
  Disk *disk;           // 物理异步磁盘设备
  Semaphore *semaphore; // 读写磁盘的信号量
  Lock *lock;           // 控制只有一个线程读写磁盘的锁
};
```

以ReadSector为例来说明同步磁盘的工作机制：

```cpp
void SynchDisk::ReadSector(int sectorNumber, char *data)
{
    lock->Acquire(); // 一次只允许一个线程访问磁盘
    disk->ReadRequest(sectorNumber, data); //请求读取磁盘
    semaphore->P();  // 等待磁盘中断的到来
    lock->Release(); // 释放锁
}
```

> 当线程向磁盘设备发出读访问请求后，等待磁盘中断的到来。一旦磁盘中断来到，中断处理程序执行semaphore->V()操作，ReadSector得以继续运行。对磁盘同步写也基于同样的原理。

#### bitmap.cc bitmap.h

> 在 Nachos 的文件系统中，是通过位图来管理空闲块的。Nachos 的物理磁盘是以扇区为访问单位的，将扇区从 0 开始编号。所谓位图管理，就是将这些编号填入一张表，表中为 0 的地 方说明该扇区没有被占用，而非 0 位置说明该扇区已被占用。这部分内容是用 BitMap 类实现的。

```cpp
class BitMap
{
public:
  BitMap(int nitems); // 初始化一个位图
  ~BitMap(); // 析构位图

  void Mark(int which);  // 将第n位设为1
  void Clear(int which); // 将第n位设为0
  bool Test(int which);  // 检查第n位是否为1
  int Find();            // 找第一个值为0的位，并置为1，如果找不到返回-1
  int NumClear(); // 返回值为0的位数
  void Print(); // 打印位图信息
  void FetchFrom(OpenFile *file); // 从Nachos磁盘读取数据到宿主主机内存
  void WriteBack(OpenFile *file); // 从宿主主机内存写回Nachos磁盘

private:
  int numBits;       // 位图的位数
  int numWords;      // 位图的字节数
  unsigned int *map; // bit以unsigned int的数据类型存储，这是bitmap的起点指针
};
```

#### filehdr.cc filehdr.h

> 文件头实际上就是 UNIX 文件系统中所说的 i-node 结构，它给出一个文件除了文件名之外的所有属性，包括文件长度、地址索引表等等（文件名属性在目录中给出）。所谓索引表，就是文件的逻辑地址和实际的物理地址的对应关系。Nachos 的文件头可以存放在磁盘上，也可以存放在宿主机内存中。在磁盘上存放时一个文件头占用一个独立的扇区。Nachos 文件 头的索引表只有直接索引。 
>
> 文件头的定义和实现如下所示，由于目前 Nachos 只支持直接索引，而且文件长度一旦固定， 就不能变动。所以文件头的实现比较简单，这里不再赘述。 

```cpp
class FileHeader
{ //i-node
public:
  bool Allocate(BitMap *bitMap, int fileSize); // 通过文件大小初始化i-node（新）
  void Deallocate(BitMap *bitMap);             // 将一个文件所占用的数据空间释放（没有释放i-node的空间）
  void FetchFrom(int sectorNumber);            // 从磁盘中取出i-node（旧）
  void WriteBack(int sectorNumber);            // 将i-node写入磁盘
  int ByteToSector(int offset);                // 实现文件逻辑地址到物理地址的转换
  int FileLength();                            // 返回文件长度
  void Print();                                // 打印文件头信息（调试用）
private:
  int numBytes;               // 文件字节数
  int numSectors;             // 文件占的扇区数
  int dataSectors[NumDirect]; // 文件索引表
};
```

按照习惯，我将Nachos的fIleHeader称为i-node，下同。

#### openfile.cc openfile.h

> 该模块定义了一个打开文件控制结构。当用户打开了一个文件时，系统即为其产生一个打开文件控制结构，以后用户对该文件的访问都可以通过该结构。打开文件控制结构中的对文件操作的方法同 UNIX 操作系统中的系统调用。 

```cpp
class OpenFile {
  public:
    OpenFile(int sector);		// 打开文件初始化方法，sector为文件头i-node的扇区号
    ~OpenFile();			    // 关闭文件
    void Seek(int position); 		// 移动文件位置指针（从头文件开始）
    int Read(char *into, int numBytes); // 从文件中读入into缓冲
    int Write(char *from, int numBytes); //从from缓冲写入文件
    int ReadAt(char *into, int numBytes, int position);//从文件position开始读
    int WriteAt(char *from, int numBytes, int position);//写入文件position开始的位置
    int Length(); 			// 返回文件长度
  private:
    FileHeader *hdr;			// 该文件对应的文件头i-node（建立关系）
    int seekPosition;			// 当前文件位置指针
};
```

#### directory.cc directory.h

> 目录在文件系统中是一个很重要的部分，它实际上是一张表，将字符形式的文件名与实际文件的文件头相对应。这样用户就能方便地通过文件名来访问文件。 Nachos 中的目录结构非常简单，它只有一级目录，也就是只有根目录；而且根目录的大小是固定的，整个文件系统中只能存放有限个文件。这样的实现比较简单，这里只介绍目录的接口： 

```cpp
class Directory
{
public:
  Directory(int size); // 初始化一张空目录，size规定了目录中存放文件个数
  ~Directory();        // 析构目录
  void FetchFrom(OpenFile *file); // 从目录inode中读入目录内容到内存
  void WriteBack(OpenFile *file); // 将该目录内容从内存写回目录inode
  int Find(char *name); // 在目录中找文件名，返回文件的i-node的物理位置
  bool Add(char *name, int newSector); // 在目录中添加一个文件
  bool Remove(char *name); // 从目录中移除一个文件
  void List();  // 打印目录信息
  void Print(); // 详细打印目录信息
private:
  int tableSize;         // 目录项数
  DirectoryEntry *table; // 目录项表
  int FindIndex(char *name); // 根据文件名找出该文件在目录中的序号
};

```

> Nachos默认初始化一张大小为10的目录，每个目录项的大小为`10 * sizeof(char) + sizeof(int) + sizeof(bool) = 15`, 十个目录项大小为150B, 超过了扇区大小128B，这是否是个bug？

#### filesys.cc filesys.h

>  读者在增强了线程管理的功能后，可以同时开展文件系统部分功能的增强或实现虚拟内存两部分工作。在 Nachos 中，实现了两套文件系统，它们对外接口是完全一样的：一套称作为 FILESYS_STUB，它是建立在 UNIX 文件系统之上的，而不使用 Nachos 的模拟磁盘，它主要用于读者先实现了用户程序和虚拟内存，然后再着手增强文件系统的功能；另一套是 Nachos 的文件系统，它是实现在 Nachos 的虚拟磁盘上的。当整个系统完成之后，只能使用第二套文件系统的实现。 

```cpp
class FileSystem
{
public:
	FileSystem(bool format);				  // 初始化文件系统
	bool Create(char *name, int initialSize); // 创造文件
	OpenFile *Open(char *name);				  // 打开文件
	bool Remove(char *name);				  // 删除文件
	void List();							  // 打印文件系统中的所有文件
	void Print();							  //详细列出文件和内容
private:
	OpenFile *freeMapFile;	 // 文件系统位图
	OpenFile *directoryFile; // 文件系统根目录
};
```

#### Exercise 2 扩展文件属性

> 增加文件描述信息，如“类型”、“创建时间”、“上次访问时间”、“上次修改时间”、“路径”等等。尝试突破文件名长度的限制。

#### 背景知识：UNIX i-node

> 1.UNIX文件系统中的主要结构 i-node ，目录项中记录的是文件名和文件相对应的 i-node 在整个 i-node 区中的索引号，文件 i-node 结构中除了存放文件的属性外，最主要的是文件的索引表。
>
> 2.磁盘存储空间的安排 在 UNIX 文件系统中，磁盘块的作用分成两类：一类存放文件的 i-node，这一类磁盘块组织 在一起，形成 i-node 区；另一类存放文件内容本身，该类的集合形成存储数据区，如图 4.7。 图中， 0#块用来存放系统的自举程序； 1#块为管理块，管理本文件系统中资源的申请和回收。 主要内容有：
>
> ![image-20201119163538243](Lab4 文件系统.assets/image-20201119163538243.png)
>
> 3. 每个Nachos 文件的 i-node 占用一个单独的扇区，分散在物理磁盘的任何地方，同一般存储扇区用同样的方式进行申请和回收。
>
>    ```
>    Nachos Disk Allocation Structure
>    
>    +----+----+---------------------+
>    | 0# | 1# | Normal Storage Area |
>    +----+----+---------------------+
>      |     |
>      |    1#: Root directory's i-node
>      |
>     0#: System bitmap file's i-node
>    ```
>
> 4. Nachos 则只有一级目录，也就是只有根目录，所有的文件都在根目录下。而且根目录中可以存放的文件数是有限的。Nachos 文件系统的根目录同样也是通过文件方式存放的，它的 i-node 占据了 1 号扇区。
>
> 5. Nachos 同一般的 UNIX 一样，采用索引表进行物理地址和逻辑地址之间的转换，索引表存放在文件的 i-node 中。但是目前 Nachos 采用的索引都是直接索引，所以 Nachos 的最大文件长度不能大于4K。

#### 新增变量

Nachos的file header等价于UNIX中的i-node，因此我将题述的几个变量加在code/filesys/filehdr.h的FileHeader类中，方便起见，设为public：

```cpp
class FileHeader
{
  ...
public:
  //lab4新增
  int createTime; //文件创造时间
  int lastVisitedTime; //文件上次被访问的时间
  int lastModifiedTime;//文件上次被修改的时间
  int type; //文件类型，0表示i-node，1表示普通文件，2表示索引文件(Exercise3)
  char *path; //文件路径
};

```

新增宏：

```cpp
//----------------------------------------------------------------------
//Lab4 Exercise2 新增成员变量
//----------------------------------------------------------------------
#define VAR_NUM  7//i-node中的变量数，变量的均占4B(int, char *), i-node中共7个变量，所以为7
#define NumDirect ((SectorSize - VAR_NUM * sizeof(int)) / sizeof(int)) // i-node中索引表大小,值为                                                                                    //（128 -  4*7）/4 = 25
#define MaxFileSize (NumDirect * SectorSize) //文件最大长度 25 * 128 = 3200B
```

#### 维护成员变量						

Nachos文件通过code/filesys/filesys.cc中的create函数创建，创建文件会调用FileHeader::Allocate()函数初始化一个i-node，应该在此函数内部增加对createTime的维护。每次访问文件（无论是读还是写），都会调用FileHeader::ByteToSector()函数来执行地址转换，所以应该在其中加入对lastVisitedTime的维护。而lastModifiedTime稍微复杂一点，只有对文件进行写操作时才会更新，所以应该在OpenFile::WriteAt()的结尾处增加对它的维护，表示一次写的结束。

以上的实现假设对i-node的修改不算作对文件本身的修改。

具体实现请查看`code/filesys/openfile.cc`和`code/filesys/filehdr.cc`。

#### 突破文件名长度的限制

文件名位于目录项中，将char[]改为char*即可。

这样一来，一个directoryEntry的大小为sizeof(char*) + sizeof(bool) + sizeof(int) = 9；一个sector可以存放 128 / 9 = 14个目录项。修改宏

### Exercise 3 扩展文件长度

> 改直接索引为间接索引，以突破文件长度不能超过4KB的限制。

在Exercise2中增加5个成员变量之后，文件的最大长度变为3200B，页表索引为25项。

### Exercise 4 实现多级目录



### Exercise 5 动态调整文件长度

> 对文件的创建操作和写入操作进行适当修改，以使其符合实习要求。 

 

## **二、文件访问的同步与互斥**

### Exercise 6 源代码阅读

> a)    阅读Nachos源代码中与异步磁盘相关的代码，理解Nachos系统中异步访问模拟磁盘的工作原理。
>
> filesys/synchdisk.h和filesys/synchdisk.cc
>
> b)    利用异步访问模拟磁盘的工作原理，在Class Console的基础上，实现Class SynchConsole。

### Exercise 7 实现文件系统的同步互斥访问机制，达到如下效果：

> a)    一个文件可以同时被多个线程访问。且每个线程独自打开文件，独自拥有一个当前文件访问位置，彼此间不会互相干扰。
>
> b)    所有对文件系统的操作必须是原子操作和序列化的。例如，当一个线程正在修改一个文件，而另一个线程正在读取该文件的内容时，读线程要么读出修改过的文件，要么读出原来的文件，不存在不可预计的中间状态。
>
> c)    当某一线程欲删除一个文件，而另外一些线程正在访问该文件时，需保证所有线程关闭了这个文件，该文件才被删除。也就是说，只要还有一个线程打开了这个文件，该文件就不能真正地被删除。

 

## 三、Challenges （至少选做1个）

### Challenge 1  性能优化

> a)    例如，为了优化寻道时间和旋转延迟时间，可以将同一文件的数据块放置在磁盘同一磁道上
>
> b)    使用cache机制减少磁盘访问次数，例如延迟写和预读取。

### Challenge 2 实现pipe机制

> 重定向openfile的输入输出方式，使得前一进程从控制台读入数据并输出至管道，后一进程从管道读入数据并输出至控制台。