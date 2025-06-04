# 音乐情绪节点游戏项目总结

## 项目概述
基于虚幻引擎5的情绪驱动节点交互游戏，通过AI动态生成故事和音乐。

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

## 核心系统架构

### 1. 节点系统 (Node System)
- **基类**: `AInteractiveNode` - 所有交互节点的基类
- **场景节点**: `ASceneNode` - 管理场景和子节点
- **物品节点**: `AItemNode` - 支持能力系统的物品节点
- **连接系统**: `ANodeConnection` - 管理节点间关系

### 2. 能力系统 (Capability System)
- **基类**: `UItemCapability` - 所有能力的基类
- **六大能力**:
  1. `USpatialCapability` - 空间能力（容纳、传送）
  2. `UStateCapability` - 状态能力（状态变化、传播）
  3. `UInteractiveCapability` - 交互能力（对话、物品交换）
  4. `UNarrativeCapability` - 叙事能力（故事推进、线索）
  5. `USystemCapability` - 系统能力（时间控制、规则修改）
  6. `UNumericalCapability` - 数值能力（血量、资源管理）

### 3. 节点关系类型
```cpp
enum class ENodeRelationType : uint8
{
    Dependency,      // 依赖关系 - StateCapability
    Prerequisite,    // 前置条件 - InteractiveCapability
    Trigger,         // 触发关系 - InteractiveCapability
    Mutual,          // 相互关联 - InteractiveCapability
    Parent,          // 父子关系 - SpatialCapability
    Sequence,        // 顺序关系 - NarrativeCapability
    Emotional        // 情绪关联 - NumericalCapability
};
```
4. 管理系统

ANodeSystemManager - 统一管理所有节点和连接
USimpleNodeDataConverter - JSON数据转换
AJSONTest - 测试Actor

##已解决的问题





##待完成功能

###能力系统测试

需要创建DataTable配置
需要在ItemNode中应用能力配置


###AI集成

故事生成API接入
音乐生成系统集成


###情绪系统深化

情绪传播机制
情绪对游戏的影响



##关键文件路径
Source/MyProject/
├── Core/NodeDataTypes.h              # 核心数据类型
├── Nodes/
│   ├── InteractiveNode.h/cpp        # 节点基类
│   ├── SceneNode.h/cpp              # 场景节点
│   ├── ItemNode.h/cpp               # 物品节点
│   ├── NodeConnection.h/cpp         # 节点连接
│   ├── NodeSystemManager.h/cpp      # 系统管理器
│   └── Capabilities/                # 六大能力
│       ├── ItemCapability.h/cpp     
│       ├── SpatialCapability.h/cpp
│       ├── StateCapability.h/cpp
│       ├── InteractiveCapability.h/cpp
│       ├── NarrativeCapability.h/cpp
│       ├── SystemCapability.h/cpp
│       └── NumericalCapability.h/cpp
├── Generation/
│   └── SimpleNodeDataConverter.h/cpp # JSON转换
└── Test/
    └── JSONTest.h/cpp               # 测试Actor
###测试配置

测试JSON: test_simple_scene.json
包含1个场景节点，3个物品节点，1个前置条件关系
