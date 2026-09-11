#include <iostream>
#include <limits>
#include <cstdlib>
#include <cmath>
#include "time_utils.h"
#include "logging.h"
using namespace std;

void fail(){
	cout << " Fail! :( \n";
}

void check(){
	if (cin.fail()){
		fail();
		cin.clear();
		cin.ignore(numeric_limits<streamsize>::max(),'\n');
		exit(0);
	}
}	

int main(){
	Logger logger("app.log");
	logger.info("程序启动");
    logger.debug("x = " + std::to_string(42));
    logger.warn("磁盘空间不足");
    logger.error("打开文件失败");
	double a,b;
	char c;
	double result = 0.0;
	getime();
	cout << "请输入第一个数字·Enter 1st number:";
	cin >> a;
	check();
	cout << "请输入第二个数字·Enter 2nd number:";
	cin >> b;
	check();
	while(true){
		cout << "你想要什么·What do you want?\n1+ 2- 3x 4/ 5幂函数\n";
		cin >> c;
		switch(c){
			case '1':       /*+*/
				result=a+b;
				break;
			case '2':       /*-*/
				result=a-b;
				break;
			case '3':       /*x*/
				result=a*b;
				break;
			case '4':
				if(fabs(b) < 1e-12){
					cout << "不能除以零·Cannot divide by zero!\n";
					return 0;
				}
				result=a/b;
				break;
			case '5':
				result=pow(a, b);
				if(std::isinf(result)){
					cout << "结果过大，无法显示!\n";
					return 0;
				}
				break;
			default:
				fail();
				cout << "[再试·Retry]";
				continue;
		}
	cout << "结果·End Number:";
	cout << result;
	return 0;
	}
}
