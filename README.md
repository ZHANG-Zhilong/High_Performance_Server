# High_Performance_Server

- 采用基于对象风格的方式设计
- 使用RAII机制包装了资源，实现了自动的资源获取和释放机制
- 使用智能指针替代几乎全部的裸指针
- 使用前向声明简化了include的依赖关系，同时减少了接口的暴露
- 利用bind代替C风格的回调函数
- 使用安全的单例模式pthread_once
- 封装了pthread库的系统调用，将其包装为更好用、更直观的api
