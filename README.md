# 节点系统游戏项目总结

## 核心概念
- 基于节点的情绪驱动游戏
- 所有游戏对象都是节点（InteractiveNode）
- 通过AI动态生成故事和音乐
- 节点间通过7种关系类型连接

## 系统架构
### 已完成
1. **节点系统**：InteractiveNode、SceneNode、ItemNode
2. **关系系统**：NodeConnection（7种关系类型）
3. **管理系统**：NodeSystemManager
4. **六大能力系统**：
   - SpatialCapability（空间）
   - StateCapability（状态）
   - InteractiveCapability（交互）
   - NarrativeCapability（叙事）
   - SystemCapability（系统）
   - NumericalCapability（数值）

### 关键文件路径
- 核心类型：`Source/MyProject/Core/NodeDataTypes.h`
- 节点类：`Source/MyProject/Nodes/`
- 能力类：`Source/MyProject/Nodes/Capabilities/`
