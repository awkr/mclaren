- screen pos to world pos 时，读取屏幕坐标，再读取depth buffer。
- BVH

随想：
- 渲染时只管拿数据，渲染完毕归还数据
- 第n帧同步准备第n帧的渲染数据
- 第n帧异步准备数据，渲染时只拿准备好的渲染数据
