/* Copyright 2023 CMU, Facebook, LANL, MIT, NVIDIA, and Stanford (alphabetical)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "dirent.h"
#include "flexflow/mapper.h"
#include "flexflow/model.h"

using namespace Legion;
using namespace FlexFlow;

// ========================================================
// Task and mapper registrations
// ========================================================
int main(int argc, char **argv) {
  // This needs to be set, otherwise NCCL will try to use group kernel launches,
  // which are not compatible with the Realm CUDA hijack.
  // 设置 NCCL 的启动模式
  setenv("NCCL_LAUNCH_MODE", "PARALLEL", true);

  // 设置 Legion 的运行时环境，定义计算执行约束和并行策略
  Runtime::set_top_level_task_id(TOP_LEVEL_TASK_ID);
  {
    TaskVariantRegistrar registrar(TOP_LEVEL_TASK_ID, "top_level");
    // 指定任务在 CPU 处理器上运行
    registrar.add_constraint(ProcessorConstraint(Processor::LOC_PROC));
    // 允许任务在多个处理器上复制执行
    registrar.set_replicable();
    Runtime::preregister_task_variant<top_level_task>(registrar, "top_level");
  }

  register_flexflow_internal_tasks();

  // Register custom tasks
  register_custom_tasks();

  // Mapper 负责将任务映射到具体的处理器，决定数据在哪个 GPU/CPU 上处理
  Runtime::add_registration_callback(FFMapper::update_mappers);

  // 将控制权交给 Legion 运行时系统
  return Runtime::start(argc, argv);
}
