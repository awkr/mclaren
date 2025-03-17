- screen pos to world pos 时，读取屏幕坐标，再读取depth buffer。
- BVH
- bindless descriptor

随想：
- 渲染时只管拿数据，渲染完毕归还数据
- 第n帧同步准备第n帧的渲染数据
- 第n帧异步准备数据，渲染时只拿准备好的渲染数据
- shadow pass的资源应该从统一的资源管理器中获取，而不是每个pass自己创建、管理资源
- 按pass拆分模块
- 基于场景aabb的大小计算shadow map的分辨率
