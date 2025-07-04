#pragma once

/* Copyright 2023 NVIDIA Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "legion.h"

using namespace Legion;

template<typename T,
         T (FN)(const Task *task,
                const std::vector<PhysicalRegion> &regions,
                Context ctx,
                Runtime *runtime)>
void regLocTask(const char *name, unsigned task_id)
{
  TaskVariantRegistrar reg(task_id, name);
  reg.add_constraint(ProcessorConstraint(Processor::LOC_PROC));
  Runtime::preregister_task_variant<T, FN>(reg, name);
}

template<void (FN)(const Task *task,
         const std::vector<PhysicalRegion> &regions,
         Context ctx,
         Runtime *runtime)>
void regLocTask(const char *name, unsigned task_id)
{
  TaskVariantRegistrar reg(task_id, name);
  reg.add_constraint(ProcessorConstraint(Processor::LOC_PROC));
#ifdef DEBUG_CTRL_REPL
  reg.set_replicable();
#endif
  Runtime::preregister_task_variant<FN>(reg, name);
}
