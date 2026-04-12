# ZOO 工程架构图（现状与规划）

## 图例

- 绿色：已实现
- 橙色：部分实现
- 蓝色：未来规划
- 黄色：外部节点
- 灰色虚线：治理与证据流

## 架构图

```mermaid
flowchart LR
  %% ===== Styles =====
  classDef ext fill:#FFF4CC,stroke:#B88A00,color:#222,stroke-width:1px;
  classDef implemented fill:#DDF4E4,stroke:#2E8B57,color:#111,stroke-width:1px;
  classDef partial fill:#FFE9C9,stroke:#C77700,color:#111,stroke-width:1px;
  classDef future fill:#E3ECFF,stroke:#2B5FB8,color:#111,stroke-width:1px;
  classDef control fill:#F2F2F2,stroke:#666,color:#111,stroke-width:1px,stroke-dasharray: 4 3;

  %% ===== External Nodes =====
  APP[应用/业务节点\nClient Server Pub Sub SHM Demo]:::ext
  OPS[运维与质量节点\nCI/CD 质量门禁 发布治理]:::ext

  %% ===== Runtime Plane =====
  subgraph RUNTIME[ZOO Runtime Plane]
    direction LR

    subgraph NODE[Node Layer]
      N1[Client Node\n请求发起 会话管理]:::implemented
      N2[Server Node\n服务暴露 请求响应]:::implemented
      N3[Publisher/Subscriber\n主题发布 订阅消费]:::implemented
    end

    subgraph CORE[Core Layer]
      C1[Bus Lifecycle\ncreate/start/stop/destroy]:::implemented
      C2[Routing Engine\n路由 分发 关联ID保持]:::implemented
      C3[Service Manager\n在线离线状态管理\n观察者通知]:::implemented
      C4[Recovery Manager/WAL\n重启恢复 持久化检查点]:::future
    end

    subgraph QOS[QoS Layer]
      Q1[Policy Validation\n可靠性/时延/资源约束]:::implemented
      Q2[Backpressure Control\ndrop block priority shed]:::implemented
      Q3[Deterministic Safety Channels\n混部隔离 最坏时延保证]:::future
    end

    subgraph TRANSPORT[Transport Layer]
      T1[TCP Transport\n连接管理 收发 事件循环]:::implemented
      T2[UDP Transport\n低开销报文传输]:::implemented
      T3[SHM Transport\n本机高吞吐低延迟]:::implemented
      T4[Version Interop Matrix\n跨传输协议版本互通]:::partial
    end

    subgraph UTIL[Utility & Platform Layer]
      U1[Platform/Socket/Thread/Timer]:::implemented
      U2[Buffer + Memory Pool\n有界内存与水位监控]:::implemented
      U3[Unified Health Contract\nliveness readiness degraded统一语义]:::partial
      U4[Observability\n日志 指标 基准与追踪]:::implemented
    end
  end

  %% ===== Governance Plane =====
  subgraph GOV[Governance & Assurance Plane]
    direction LR
    G1[Requirements Traceability\n需求到测试矩阵]:::implemented
    G2[Threat Model + Vuln SLA\n威胁建模 漏洞闭环]:::implemented
    G3[Safety Case & Freeze Procedure\n安全论证骨架 证据冻结]:::partial
    G4[External Pre-assessment\n外部评估与整改闭环]:::future
  end

  %% ===== Data Flow =====
  APP -->|业务消息/命令| N1
  APP -->|服务请求| N2
  APP -->|主题数据| N3

  N1 --> C2
  N2 --> C2
  N3 --> C2

  C1 --> C2
  C3 --> C2
  C2 --> Q1
  Q1 --> Q2

  Q2 --> T1
  Q2 --> T2
  Q2 --> T3

  T1 -->|网络数据| APP
  T2 -->|网络数据| APP
  T3 -->|共享内存数据| APP

  T1 --> U1
  T2 --> U1
  T3 --> U1
  U1 --> U2
  U2 --> U4
  U3 -.标准化健康状态.-> C1

  %% ===== Control/Evidence Flow =====
  OPS -.质量门禁/发布规则.-> GOV
  GOV -.约束与证据回灌.-> RUNTIME
  G1 -.需求约束.-> C1
  G2 -.安全约束.-> T1
  G3 -.发布准入.-> OPS

  %% ===== Future links =====
  C4 -.恢复路径.-> C1
  Q3 -.安全通道策略.-> Q1
  T4 -.互通验证.-> T1
  T4 -.互通验证.-> T2
  T4 -.互通验证.-> T3
  G4 -.外部评估结论.-> G3
```

## 功能状态说明

- 已实现：Node 层（Client/Server/PubSub）、核心生命周期与路由、TCP/UDP/SHM 传输、回压策略、有界内存与可观测性、需求追踪与安全流程基线。
- 部分实现：跨传输协议版本互通验证、统一健康语义、安全证据流程落地执行。
- 未来实现：恢复管理（WAL/Checkpoint）、确定性安全通道、外部预评估与整改闭环。
