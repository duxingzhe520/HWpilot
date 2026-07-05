### 一、强制类型转换
#### C++的cast运算符
- `static_cast`
	- 用于比较“自然”和低风险的转换
		- 整形 & 实数型，字符型
		- 派生类转基类
	- 不能用于
		- 不同类型指针的转换
		- 整形和指针的转换
		- 不同类型引用的转换
```cpp
int n; char* p = "hello";
n = static_cast<int>(3.14);
p = static_cast<char*>(a);
```
- `dynamic_cast`
	- 将==多态==基类的指针或引用转换为派生类的指针或引用
- `reinterpret_cast`
	- 不同类型的指针的转换
	- 不同类型的引用的转换
	- 执行的是逐个bit拷贝的操作
- `const_cast`
	- 去除`const`属性的转换
```cpp
const string s = "Inception";
string& p = const_cast<string&>(s);
string* ps = const_cast<string*>(&s);
```

### 二、异常处理
- `throw`语句
	- `throw + 表达式`
- `try ... catch`语句
	- `catch(...)`可以接收任何类型的异常
- C++标准异常类（继承自exception）
	- `bad_tyepid`
	- `bad_cast`
	- `bad_alloc`
	- `ios_base::failure`
	- `logic_error`

### 三、运行时类型检查
- `typeid`单目运算符
	- 返回值是一个`type_info`类的对象
	- 多态时返回的是实际指向的类型

###  四、多文件
#### 命名空间
```cpp
namespace group1 {
	class A {
		...;
	}
}
group1::A a;
using namespace group1;
// 后面会覆盖这个命名空间

using std::cout;
using std::vector;
using std::endl;
```

#### 预编译
真正开始在程序编译前，对源代码进行处理，主要包含：
- 符号常量定义和宏定义
- 文件包含
- 条件编译
- ```cpp
  #define DEBUG_VERSION
  #ifdef DEBUG_VERSION
	  第一部分
  #else
	  第二部分
  #endif
  ```
- 共享全局变量 / 函数 `extern int a;`，`extern void fun();`














