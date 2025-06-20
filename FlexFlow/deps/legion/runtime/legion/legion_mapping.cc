/* Copyright 2023 Stanford University, NVIDIA Corporation
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

#include "legion.h"
#include "legion/region_tree.h"
#include "legion/legion_views.h"
#include "legion/legion_mapping.h"
#include "legion/mapper_manager.h"
#include "legion/legion_instances.h"

namespace Legion {
  namespace Mapping {

    /////////////////////////////////////////////////////////////
    // PhysicalInstance 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    PhysicalInstance::PhysicalInstance(void)
      : impl(NULL)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    PhysicalInstance::PhysicalInstance(PhysicalInstanceImpl i)
      : impl(i)
    //--------------------------------------------------------------------------
    {
      // By holding resource references, we prevent the data
      // structure from being collected, it doesn't change if 
      // the actual instance itself can be collected or not
      if (impl != NULL)
        impl->add_base_resource_ref(Internal::MAPPER_REF);
    }

    //--------------------------------------------------------------------------
    PhysicalInstance::PhysicalInstance(PhysicalInstance &&rhs)
      : impl(rhs.impl)
    //--------------------------------------------------------------------------
    {
      rhs.impl = NULL;
    }

    //--------------------------------------------------------------------------
    PhysicalInstance::PhysicalInstance(const PhysicalInstance &rhs)
      : impl(rhs.impl)
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
        impl->add_base_resource_ref(Internal::MAPPER_REF);
    }

    //--------------------------------------------------------------------------
    PhysicalInstance::~PhysicalInstance(void)
    //--------------------------------------------------------------------------
    {
      if ((impl != NULL) && 
          impl->remove_base_resource_ref(Internal::MAPPER_REF))
        delete (impl);
    }

    //--------------------------------------------------------------------------
    PhysicalInstance& PhysicalInstance::operator=(PhysicalInstance &&rhs)
    //--------------------------------------------------------------------------
    {
      if ((impl != NULL) && 
          impl->remove_base_resource_ref(Internal::MAPPER_REF))
        delete (impl);
      impl = rhs.impl;
      rhs.impl = NULL;
      return *this;
    }

    //--------------------------------------------------------------------------
    PhysicalInstance& PhysicalInstance::operator=(const PhysicalInstance &rhs)
    //--------------------------------------------------------------------------
    {
      if ((impl != NULL) && 
          impl->remove_base_resource_ref(Internal::MAPPER_REF))
        delete (impl);
      impl = rhs.impl;
      if (impl != NULL)
        impl->add_base_resource_ref(Internal::MAPPER_REF);
      return *this;
    }

    //--------------------------------------------------------------------------
    bool PhysicalInstance::operator<(const PhysicalInstance &rhs) const
    //--------------------------------------------------------------------------
    {
      return (impl < rhs.impl);
    }

    //--------------------------------------------------------------------------
    bool PhysicalInstance::operator==(const PhysicalInstance &rhs) const
    //--------------------------------------------------------------------------
    {
      return (impl == rhs.impl);
    }

    //--------------------------------------------------------------------------
    bool PhysicalInstance::operator!=(const PhysicalInstance &rhs) const
    //--------------------------------------------------------------------------
    {
      return (impl != rhs.impl);
    }

    //--------------------------------------------------------------------------
    Memory PhysicalInstance::get_location(void) const
    //--------------------------------------------------------------------------
    {
      if ((impl == NULL) || !impl->is_physical_manager())
        return Memory::NO_MEMORY;
      Internal::PhysicalManager *manager = impl->as_physical_manager();
      return manager->get_memory();
    }

    //--------------------------------------------------------------------------
    unsigned long PhysicalInstance::get_instance_id(void) const
    //--------------------------------------------------------------------------
    {
      if ((impl == NULL) || !impl->is_physical_manager())
        return 0;
      Internal::PhysicalManager *manager = impl->as_physical_manager();
      return manager->get_instance().id;
    }

    //--------------------------------------------------------------------------
    size_t PhysicalInstance::get_instance_size(void) const
    //--------------------------------------------------------------------------
    {
      if ((impl == NULL) || !impl->is_physical_manager())
        return 0;
      return impl->as_physical_manager()->get_instance_size();
    }

    //--------------------------------------------------------------------------
    Domain PhysicalInstance::get_instance_domain(void) const
    //--------------------------------------------------------------------------
    {
      if ((impl == NULL) || !impl->is_physical_manager())
        return Domain::NO_DOMAIN;
      Domain domain;
      impl->instance_domain->get_domain(domain);
      return domain;
    }

    //--------------------------------------------------------------------------
    FieldSpace PhysicalInstance::get_field_space(void) const
    //--------------------------------------------------------------------------
    {
      if ((impl == NULL) || !impl->is_physical_manager())
        return FieldSpace::NO_SPACE;
      return impl->field_space_node->handle;
    }

    //--------------------------------------------------------------------------
    RegionTreeID PhysicalInstance::get_tree_id(void) const
    //--------------------------------------------------------------------------
    {
      if ((impl == NULL) || !impl->is_physical_manager())
        return 0;
      return impl->tree_id;
    }

    //--------------------------------------------------------------------------
    LayoutConstraintID PhysicalInstance::get_layout_id(void) const
    //--------------------------------------------------------------------------
    {
      if ((impl == NULL) || !impl->is_physical_manager())
        return 0;
      return impl->layout->constraints->layout_id;
    }

    //--------------------------------------------------------------------------
    bool PhysicalInstance::exists(bool strong_test /*= false*/) const
    //--------------------------------------------------------------------------
    {
      if ((impl == NULL) || !impl->is_physical_manager())
        return false;
      // Check to see if it still exists for now, maybe in the future
      // we could do a full check to see if it still exists on its owner node
      if (strong_test)
        assert(false); // implement this
      return true;
    }

    //--------------------------------------------------------------------------
    bool PhysicalInstance::is_normal_instance(void) const
    //--------------------------------------------------------------------------
    {
      if ((impl == NULL) || !impl->is_physical_manager())
        return false;
      return !impl->is_reduction_manager();
    }

    //--------------------------------------------------------------------------
    bool PhysicalInstance::is_virtual_instance(void) const
    //--------------------------------------------------------------------------
    {
      if (impl == NULL)
        return false;
      return impl->is_virtual_manager();
    }

    //--------------------------------------------------------------------------
    bool PhysicalInstance::is_reduction_instance(void) const
    //--------------------------------------------------------------------------
    {
      if ((impl == NULL) || !impl->is_physical_manager())
        return false;
      return impl->is_reduction_manager();
    }

    //--------------------------------------------------------------------------
    bool PhysicalInstance::is_external_instance(void) const
    //--------------------------------------------------------------------------
    {
      if ((impl == NULL) || !impl->is_physical_manager())
        return false;
      return impl->is_external_instance();
    }

    //--------------------------------------------------------------------------
    /*static*/ PhysicalInstance PhysicalInstance::get_virtual_instance(void)
    //--------------------------------------------------------------------------
    {
      return PhysicalInstance(Internal::implicit_runtime->virtual_manager);
    }

    //--------------------------------------------------------------------------
    void PhysicalInstance::get_fields(std::set<FieldID> &fields) const
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
        impl->get_fields(fields);
    }

    //--------------------------------------------------------------------------
    bool PhysicalInstance::has_field(FieldID fid) const
    //--------------------------------------------------------------------------
    {
      if (impl == NULL)
        return false;
      return impl->has_field(fid);
    }

    //--------------------------------------------------------------------------
    void PhysicalInstance::has_fields(std::map<FieldID,bool> &fields) const
    //--------------------------------------------------------------------------
    {
      if (impl == NULL)
      {
        for (std::map<FieldID,bool>::iterator it = fields.begin();
              it != fields.end(); it++)
          it->second = false;
        return;
      }
      return impl->has_fields(fields);
    }

    //--------------------------------------------------------------------------
    void PhysicalInstance::remove_space_fields(std::set<FieldID> &fields) const
    //--------------------------------------------------------------------------
    {
      if (impl == NULL)
        return;
      impl->remove_space_fields(fields);
    }

    //--------------------------------------------------------------------------
    bool PhysicalInstance::entails(const LayoutConstraintSet &constraint_set,
                               const LayoutConstraint **failed_constraint) const 
    //--------------------------------------------------------------------------
    {
      if (impl == NULL)
        return false;
      return impl->entails(constraint_set, failed_constraint);
    }

    //--------------------------------------------------------------------------
    /*friend*/ std::ostream& operator<<(std::ostream& os,
					const PhysicalInstance& p)
    //--------------------------------------------------------------------------
    {
      if (!p.impl->is_physical_manager())
        return os << Realm::RegionInstance::NO_INST;
      else
        return os << p.impl->as_physical_manager()->get_instance();
    }

    /////////////////////////////////////////////////////////////
    // CollectiveView
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    CollectiveView::CollectiveView(void)
      : impl(NULL)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    CollectiveView::CollectiveView(CollectiveViewImpl i)
      : impl(i)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(impl != NULL);
#endif
      impl->add_base_gc_ref(Internal::MAPPER_REF);
    }

    //--------------------------------------------------------------------------
    CollectiveView::CollectiveView(CollectiveView &&rhs)
      : impl(rhs.impl)
    //--------------------------------------------------------------------------
    {
      rhs.impl = NULL;
    }

    //--------------------------------------------------------------------------
    CollectiveView::CollectiveView(const CollectiveView &rhs)
      : impl(rhs.impl)
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
        impl->add_base_gc_ref(Internal::MAPPER_REF);
    }

    //--------------------------------------------------------------------------
    CollectiveView::~CollectiveView(void)
    //--------------------------------------------------------------------------
    {
      if ((impl != NULL) &&
          impl->remove_base_gc_ref(Internal::MAPPER_REF))
        delete impl;
    }

    //--------------------------------------------------------------------------
    CollectiveView& CollectiveView::operator=(CollectiveView &&rhs)
    //--------------------------------------------------------------------------
    {
      if ((impl != NULL) &&
          impl->remove_base_gc_ref(Internal::MAPPER_REF))
        delete impl;
      impl = rhs.impl;
      rhs.impl = NULL;
      return *this;
    }

    //--------------------------------------------------------------------------
    CollectiveView& CollectiveView::operator=(const CollectiveView &rhs)
    //--------------------------------------------------------------------------
    {
      if ((impl != NULL) &&
          impl->remove_base_gc_ref(Internal::MAPPER_REF))
        delete impl;
      impl = rhs.impl;
      if (impl != NULL)
        impl->add_base_gc_ref(Internal::MAPPER_REF);
      return *this;
    }

    //--------------------------------------------------------------------------
    bool CollectiveView::operator<(const CollectiveView &rhs) const
    //--------------------------------------------------------------------------
    {
      return (impl < rhs.impl);
    }

    //--------------------------------------------------------------------------
    bool CollectiveView::operator==(const CollectiveView &rhs) const
    //--------------------------------------------------------------------------
    {
      return (impl == rhs.impl);
    }

    //--------------------------------------------------------------------------
    bool CollectiveView::operator!=(const CollectiveView &rhs) const
    //--------------------------------------------------------------------------
    {
      return (impl != rhs.impl);
    }

    //--------------------------------------------------------------------------
    void CollectiveView::find_instances_in_memory(Memory memory,
                                     std::vector<PhysicalInstance> &insts) const
    //--------------------------------------------------------------------------
    {
      if (impl == NULL)
        return;
      std::vector<Internal::PhysicalManager*> managers;
      impl->find_instances_in_memory(memory, managers);
      insts.reserve(insts.size() + managers.size());
      for (unsigned idx = 0; idx < managers.size(); idx++)
        insts.emplace_back(PhysicalInstance(managers[idx]));
    }

    //--------------------------------------------------------------------------
    void CollectiveView::find_instances_nearest_memory(Memory memory,
                     std::vector<PhysicalInstance> &insts, bool bandwidth) const
    //--------------------------------------------------------------------------
    {
      if (impl == NULL)
        return;
      std::vector<Internal::PhysicalManager*> managers;
      impl->find_instances_nearest_memory(memory, managers, bandwidth);
      insts.reserve(insts.size() + managers.size());
      for (unsigned idx = 0; idx < managers.size(); idx++)
        insts.emplace_back(PhysicalInstance(managers[idx]));
    }

    //--------------------------------------------------------------------------
    /*friend*/ std::ostream& operator<<(std::ostream& os,
					const CollectiveView &v)
    //--------------------------------------------------------------------------
    {
      if (v.impl == NULL)
        return os << "Empty Collective View";
      else
        return os << "Collective View " << std::hex << v.impl->did << std::dec;
    }

    /////////////////////////////////////////////////////////////
    // ProfilingRequest
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    void ProfilingRequest::populate_realm_profiling_request(
					      Realm::ProfilingRequest& req)
    //--------------------------------------------------------------------------
    {
      for (std::set<ProfilingMeasurementID>::const_iterator it =
	     requested_measurements.begin();
	   it != requested_measurements.end();
	   ++it)
      {
	if((int)(*it) <= (int)(Realm::PMID_REALM_LAST))
	  req.add_measurement((Realm::ProfilingMeasurementID)(*it));
      }
    }

    /////////////////////////////////////////////////////////////
    // ProfilingResponse
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    void ProfilingResponse::attach_realm_profiling_response(
					const Realm::ProfilingResponse& resp)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(realm_resp == NULL);
#endif
      realm_resp = &resp;
    }
    
    //--------------------------------------------------------------------------
    void ProfilingResponse::attach_overhead(
                                   ProfilingMeasurements::RuntimeOverhead *over)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(overhead == NULL);
#endif
      overhead = over; 
    }

    /////////////////////////////////////////////////////////////
    // Mapper 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    Mapper::Mapper(MapperRuntime *rt)
      : runtime(rt)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    Mapper::~Mapper(void)
    //--------------------------------------------------------------------------
    {
    }

    LEGION_DISABLE_DEPRECATED_WARNINGS

    //--------------------------------------------------------------------------
    Mapper::PremapTaskInput::PremapTaskInput(void)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    Mapper::PremapTaskInput::~PremapTaskInput(void)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    Mapper::PremapTaskOutput::PremapTaskOutput(void)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    Mapper::PremapTaskOutput::~PremapTaskOutput(void)
    //--------------------------------------------------------------------------
    {
    }

    LEGION_REENABLE_DEPRECATED_WARNINGS

    /////////////////////////////////////////////////////////////
    // MapperRuntime
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    MapperRuntime::MapperRuntime(void)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    MapperRuntime::~MapperRuntime(void)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    bool MapperRuntime::is_locked(MapperContext ctx) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->is_locked(ctx);
    }

    //--------------------------------------------------------------------------
    void MapperRuntime::lock_mapper(MapperContext ctx, bool read_only) const
    //--------------------------------------------------------------------------
    {
      ctx->manager->lock_mapper(ctx, read_only);
    }

    //--------------------------------------------------------------------------
    void MapperRuntime::unlock_mapper(MapperContext ctx) const
    //--------------------------------------------------------------------------
    {
      ctx->manager->unlock_mapper(ctx);
    }

    //--------------------------------------------------------------------------
    bool MapperRuntime::is_reentrant(MapperContext ctx) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->is_reentrant(ctx);
    }

    //--------------------------------------------------------------------------
    void MapperRuntime::enable_reentrant(MapperContext ctx) const
    //--------------------------------------------------------------------------
    {
      ctx->manager->enable_reentrant(ctx);
    }

    //--------------------------------------------------------------------------
    void MapperRuntime::disable_reentrant(MapperContext ctx) const
    //--------------------------------------------------------------------------
    {
      ctx->manager->disable_reentrant(ctx);
    }

    //--------------------------------------------------------------------------
    void MapperRuntime::update_mappable_tag(MapperContext ctx,
                               const Mappable &mappable, MappingTagID tag) const
    //--------------------------------------------------------------------------
    {
      ctx->manager->update_mappable_tag(ctx, mappable, tag);
    }

    //--------------------------------------------------------------------------
    void MapperRuntime::update_mappable_data(MapperContext ctx,
      const Mappable &mappable, const void *mapper_data, size_t data_size) const
    //--------------------------------------------------------------------------
    {
      ctx->manager->update_mappable_data(ctx, mappable, mapper_data, data_size);
    }

    //--------------------------------------------------------------------------
    void MapperRuntime::send_message(MapperContext ctx, Processor target,
          const void *message, size_t message_size, unsigned message_kind) const
    //--------------------------------------------------------------------------
    {
      ctx->manager->send_message(ctx, target, 
                                 message, message_size, message_kind);
    }

    //--------------------------------------------------------------------------
    void MapperRuntime::broadcast(MapperContext ctx, const void *message,
                    size_t message_size, unsigned message_kind, int radix) const
    //--------------------------------------------------------------------------
    {
      ctx->manager->broadcast(ctx, message, message_size, message_kind, radix);
    }

    //--------------------------------------------------------------------------
    void MapperRuntime::pack_physical_instance(MapperContext ctx, 
                               Serializer &rez, PhysicalInstance instance) const
    //--------------------------------------------------------------------------
    {
      ctx->manager->pack_physical_instance(ctx, rez, instance);
    }

    //--------------------------------------------------------------------------
    void MapperRuntime::unpack_physical_instance(MapperContext ctx,
                          Deserializer &derez, PhysicalInstance &instance) const
    //--------------------------------------------------------------------------
    {
      ctx->manager->unpack_physical_instance(ctx, derez, instance);
    }

    //--------------------------------------------------------------------------
    MapperEvent MapperRuntime::create_mapper_event(MapperContext ctx) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->create_mapper_event(ctx);
    }

    //--------------------------------------------------------------------------
    bool MapperRuntime::has_mapper_event_triggered(MapperContext ctx,
                                                      MapperEvent event) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->has_mapper_event_triggered(ctx, event);
    }

    //--------------------------------------------------------------------------
    void MapperRuntime::trigger_mapper_event(MapperContext ctx, 
                                                MapperEvent event) const
    //--------------------------------------------------------------------------
    {
      ctx->manager->trigger_mapper_event(ctx, event);
    }

    //--------------------------------------------------------------------------
    void MapperRuntime::wait_on_mapper_event(MapperContext ctx,
                                                MapperEvent event) const
    //--------------------------------------------------------------------------
    {
      ctx->manager->wait_on_mapper_event(ctx, event);
    }

    //--------------------------------------------------------------------------
    const ExecutionConstraintSet& MapperRuntime::find_execution_constraints(
                         MapperContext ctx, TaskID task_id, VariantID vid) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->find_execution_constraints(ctx, task_id, vid);
    }

    //--------------------------------------------------------------------------
    const TaskLayoutConstraintSet& 
      MapperRuntime::find_task_layout_constraints(MapperContext ctx, 
                                            TaskID task_id, VariantID vid) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->find_task_layout_constraints(ctx, task_id, vid);
    }

    //--------------------------------------------------------------------------
    const LayoutConstraintSet&
      MapperRuntime::find_layout_constraints(MapperContext ctx, 
                                                LayoutConstraintID id) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->find_layout_constraints(ctx, id); 
    }

    //--------------------------------------------------------------------------
    LayoutConstraintID MapperRuntime::register_layout(MapperContext ctx,
                                   const LayoutConstraintSet &constraints,
                                   FieldSpace handle) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->register_layout(ctx, constraints, handle);
    }

    //--------------------------------------------------------------------------
    void MapperRuntime::release_layout(MapperContext ctx,
                                          LayoutConstraintID layout_id) const
    //--------------------------------------------------------------------------
    {
      ctx->manager->release_layout(ctx, layout_id);
    }

    //--------------------------------------------------------------------------
    bool MapperRuntime::do_constraints_conflict(MapperContext ctx,
                             LayoutConstraintID set1, LayoutConstraintID set2,
                             const LayoutConstraint **conflict_constraint) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->do_constraints_conflict(ctx, set1, set2,
                                                   conflict_constraint);
    }

    //--------------------------------------------------------------------------
    bool MapperRuntime::do_constraints_entail(MapperContext ctx,
                           LayoutConstraintID source, LayoutConstraintID target,
                           const LayoutConstraint **failed_constraint) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->do_constraints_entail(ctx, source, target, 
                                                 failed_constraint);
    }

    //--------------------------------------------------------------------------
    void MapperRuntime::find_valid_variants(MapperContext ctx,TaskID task_id,
                                         std::vector<VariantID> &valid_variants,
                                         Processor::Kind kind) const
    //--------------------------------------------------------------------------
    {
      ctx->manager->find_valid_variants(ctx, task_id, valid_variants, kind);
    }

    //--------------------------------------------------------------------------
    const char* MapperRuntime::find_task_variant_name(MapperContext ctx,
                                     TaskID task_id, VariantID variant_id) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->find_task_variant_name(ctx, task_id, variant_id);
    }

    //--------------------------------------------------------------------------
    bool MapperRuntime::is_leaf_variant(MapperContext ctx, TaskID task_id,
                                           VariantID variant_id) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->is_leaf_variant(ctx, task_id, variant_id);
    }

    //--------------------------------------------------------------------------
    bool MapperRuntime::is_inner_variant(MapperContext ctx, TaskID task_id,
                                            VariantID variant_id) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->is_inner_variant(ctx, task_id, variant_id);
    }

    //--------------------------------------------------------------------------
    bool MapperRuntime::is_idempotent_variant(MapperContext ctx, 
                                     TaskID task_id, VariantID variant_id) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->is_idempotent_variant(ctx, task_id, variant_id);
    }

    //--------------------------------------------------------------------------
    bool MapperRuntime::is_replicable_variant(MapperContext ctx,
                                     TaskID task_id, VariantID variant_id) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->is_replicable_variant(ctx, task_id, variant_id);
    }

    //--------------------------------------------------------------------------
    VariantID MapperRuntime::register_task_variant(MapperContext ctx,
                                      const TaskVariantRegistrar &registrar,
				      const CodeDescriptor &codedesc,
				      const void *user_data, size_t user_len,
                                      size_t return_type_size, 
                                      bool has_return_type)
    //--------------------------------------------------------------------------
    {
      return ctx->manager->register_task_variant(ctx, registrar, codedesc,
                    user_data, user_len, return_type_size, has_return_type);
    }

    //--------------------------------------------------------------------------
    void MapperRuntime::filter_variants(MapperContext ctx, const Task &task,
            const std::vector<std::vector<PhysicalInstance> > &chosen_instances,
                                           std::vector<VariantID> &variants)
    //--------------------------------------------------------------------------
    {
      ctx->manager->filter_variants(ctx, task, chosen_instances, variants);
    }

    //--------------------------------------------------------------------------
    void MapperRuntime::filter_instances(MapperContext ctx, const Task &task,
                                      VariantID chosen_variant, 
                        std::vector<std::vector<PhysicalInstance> > &instances,
                               std::vector<std::set<FieldID> > &missing_fields)
    //--------------------------------------------------------------------------
    {
      ctx->manager->filter_instances(ctx, task, chosen_variant, 
                                     instances, missing_fields);
    }

    //--------------------------------------------------------------------------
    void MapperRuntime::filter_instances(MapperContext ctx, const Task &task,
                                      unsigned index, VariantID chosen_variant,
                                      std::vector<PhysicalInstance> &instances,
                                      std::set<FieldID> &missing_fields)
    //--------------------------------------------------------------------------
    {
      ctx->manager->filter_instances(ctx, task, index, chosen_variant,
                                     instances, missing_fields);
    }

    //--------------------------------------------------------------------------
    bool MapperRuntime::create_physical_instance(
                                    MapperContext ctx, Memory target_memory,
                                    const LayoutConstraintSet &constraints, 
                                    const std::vector<LogicalRegion> &regions,
                                    PhysicalInstance &result, 
                                    bool acquire, GCPriority priority,
                                    bool tight_bounds, size_t *footprint,
                                    const LayoutConstraint **unsat) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->create_physical_instance(ctx, target_memory, 
                                 constraints, regions, result, acquire, 
                                 priority, tight_bounds, footprint, unsat);
    }

    //--------------------------------------------------------------------------
    bool MapperRuntime::create_physical_instance(
                                    MapperContext ctx, Memory target_memory,
                                    LayoutConstraintID layout_id,
                                    const std::vector<LogicalRegion> &regions,
                                    PhysicalInstance &result,
                                    bool acquire, GCPriority priority,
                                    bool tight_bounds, size_t *footprint,
                                    const LayoutConstraint **unsat) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->create_physical_instance(ctx, target_memory, 
                                   layout_id, regions, result, acquire, 
                                   priority, tight_bounds, footprint, unsat);
    }

    //--------------------------------------------------------------------------
    bool MapperRuntime::find_or_create_physical_instance(
                                    MapperContext ctx, Memory target_memory,
                                    const LayoutConstraintSet &constraints, 
                                    const std::vector<LogicalRegion> &regions, 
                                    PhysicalInstance &result, bool &created, 
                                    bool acquire, GCPriority priority,
                                    bool tight_bounds, size_t *footprint,
                                    const LayoutConstraint **unsat) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->find_or_create_physical_instance(ctx, target_memory, 
                                       constraints, regions, result, created, 
                                       acquire, priority, tight_bounds,
                                       footprint, unsat);
    }

    //--------------------------------------------------------------------------
    bool MapperRuntime::find_or_create_physical_instance(
                                    MapperContext ctx, Memory target_memory,
                                    LayoutConstraintID layout_id,
                                    const std::vector<LogicalRegion> &regions,
                                    PhysicalInstance &result, bool &created, 
                                    bool acquire, GCPriority priority,
                                    bool tight_bounds, size_t *footprint,
                                    const LayoutConstraint **unsat) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->find_or_create_physical_instance(ctx, target_memory,
                                  layout_id, regions, result, created, acquire, 
                                  priority, tight_bounds, footprint, unsat);
    }

    //--------------------------------------------------------------------------
    bool MapperRuntime::find_physical_instance(
                                    MapperContext ctx, Memory target_memory,
                                    const LayoutConstraintSet &constraints,
                                    const std::vector<LogicalRegion> &regions,
                                    PhysicalInstance &result,
                                    bool acquire, bool tight_bounds) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->find_physical_instance(ctx, target_memory, 
                constraints, regions, result, acquire, tight_bounds);
    }

    //--------------------------------------------------------------------------
    bool MapperRuntime::find_physical_instance(
                                    MapperContext ctx, Memory target_memory,
                                    LayoutConstraintID layout_id,
                                    const std::vector<LogicalRegion> &regions, 
                                    PhysicalInstance &result,
                                    bool acquire, bool tight_bounds) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->find_physical_instance(ctx, target_memory,
                  layout_id, regions, result, acquire, tight_bounds);
    }

    //--------------------------------------------------------------------------
    void MapperRuntime::find_physical_instances(
                                    MapperContext ctx, Memory target_memory,
                                    const LayoutConstraintSet &constraints,
                                    const std::vector<LogicalRegion> &regions,
                                    std::vector<PhysicalInstance> &results,
                                    bool acquire, bool tight_bounds) const
    //--------------------------------------------------------------------------
    {
      ctx->manager->find_physical_instances(ctx, target_memory, constraints, 
                                    regions, results, acquire, tight_bounds);
    }

    //--------------------------------------------------------------------------
    void MapperRuntime::find_physical_instances(
                                    MapperContext ctx, Memory target_memory,
                                    LayoutConstraintID layout_id,
                                    const std::vector<LogicalRegion> &regions, 
                                    std::vector<PhysicalInstance> &results,
                                    bool acquire, bool tight_bounds) const
    //--------------------------------------------------------------------------
    {
      ctx->manager->find_physical_instances(ctx, target_memory, layout_id, 
                                  regions, results, acquire, tight_bounds);
    }

    //--------------------------------------------------------------------------
    void MapperRuntime::set_garbage_collection_priority(MapperContext ctx,
                    const PhysicalInstance &instance, GCPriority priority) const
    //--------------------------------------------------------------------------
    {
      ctx->manager->set_garbage_collection_priority(ctx, instance, priority);
    }

    //--------------------------------------------------------------------------
    bool MapperRuntime::acquire_instance(MapperContext ctx, 
                                         const PhysicalInstance &instance) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->acquire_instance(ctx, instance);
    }

    //--------------------------------------------------------------------------
    bool MapperRuntime::acquire_instances(MapperContext ctx,
                          const std::vector<PhysicalInstance> &instances) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->acquire_instances(ctx, instances);
    }

    //--------------------------------------------------------------------------
    bool MapperRuntime::acquire_and_filter_instances(MapperContext ctx,
           std::vector<PhysicalInstance> &instances, bool filter_acquired) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->acquire_and_filter_instances(ctx, instances,
                                                        filter_acquired);
    }

    //--------------------------------------------------------------------------
    bool MapperRuntime::acquire_instances(MapperContext ctx,
            const std::vector<std::vector<PhysicalInstance> > &instances) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->acquire_instances(ctx, instances);
    }

    //--------------------------------------------------------------------------
    bool MapperRuntime::acquire_and_filter_instances(MapperContext ctx,
                  std::vector<std::vector<PhysicalInstance> > &instances,
                  bool filter_acquired_instances) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->acquire_and_filter_instances(ctx, instances,
                                            filter_acquired_instances);
    }

    //--------------------------------------------------------------------------
    void MapperRuntime::release_instance(MapperContext ctx, 
                                         const PhysicalInstance &instance) const
    //--------------------------------------------------------------------------
    {
      ctx->manager->release_instance(ctx, instance);
    }

    //--------------------------------------------------------------------------
    void MapperRuntime::release_instances(MapperContext ctx,
                          const std::vector<PhysicalInstance> &instances) const
    //--------------------------------------------------------------------------
    {
      ctx->manager->release_instances(ctx, instances);
    }

    //--------------------------------------------------------------------------
    void MapperRuntime::release_instances(MapperContext ctx,
            const std::vector<std::vector<PhysicalInstance> > &instances) const
    //--------------------------------------------------------------------------
    {
      ctx->manager->release_instances(ctx, instances);
    }

    //--------------------------------------------------------------------------
    bool MapperRuntime::acquire_future(MapperContext ctx,
                                       const Future &f, Memory memory) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->acquire_future(ctx, f, memory);
    }

    //--------------------------------------------------------------------------
    IndexSpace MapperRuntime::create_index_space(MapperContext ctx, 
           const Domain &bounds, TypeTag type_tag, const char *provenance) const
    //--------------------------------------------------------------------------
    {
      if (type_tag == 0)
      {
        switch (bounds.get_dim())
        {
#define DIMFUNC(DIM) \
          case DIM: \
            { \
              type_tag = \
                Legion::Internal::NT_TemplateHelper::encode_tag<DIM,coord_t>(); \
              break; \
            }
          LEGION_FOREACH_N(DIMFUNC)
#undef DIMFUNC
          default:
            assert(false);
        }
      }
      return ctx->manager->create_index_space(ctx, bounds, type_tag,provenance);
    }

    //--------------------------------------------------------------------------
    IndexSpace MapperRuntime::create_index_space(MapperContext ctx,
           const std::vector<DomainPoint> &points, const char *provenance) const
    //--------------------------------------------------------------------------
    {
      switch (points[0].get_dim())
      {
#define DIMFUNC(DIM) \
      case DIM: \
        { \
          std::vector<Realm::Point<DIM,coord_t> > realm_points(points.size()); \
          for (unsigned idx = 0; idx < points.size(); idx++) \
            realm_points[idx] = Point<DIM,coord_t>(points[idx]); \
          DomainT<DIM,coord_t> realm_is( \
              (Realm::IndexSpace<DIM,coord_t>(realm_points))); \
          const Domain domain(realm_is); \
          return ctx->manager->create_index_space(ctx, domain, \
           Internal::NT_TemplateHelper::encode_tag<DIM,coord_t>(),provenance); \
        }
        LEGION_FOREACH_N(DIMFUNC)
#undef DIMFUNC
        default:
          assert(false);
      }
      return IndexSpace::NO_SPACE;
    }

    //--------------------------------------------------------------------------
    IndexSpace MapperRuntime::create_index_space(MapperContext ctx,
                 const std::vector<Domain> &rects, const char *provenance) const
    //--------------------------------------------------------------------------
    {
      switch (rects[0].get_dim())
      {
#define DIMFUNC(DIM) \
        case DIM: \
          { \
            std::vector<Realm::Rect<DIM,coord_t> > realm_rects(rects.size()); \
            for (unsigned idx = 0; idx < rects.size(); idx++) \
              realm_rects[idx] = Rect<DIM,coord_t>(rects[idx]); \
            DomainT<DIM,coord_t> realm_is( \
                (Realm::IndexSpace<DIM,coord_t>(realm_rects))); \
            const Domain domain(realm_is); \
            return ctx->manager->create_index_space(ctx, domain, \
                      Internal::NT_TemplateHelper::encode_tag<DIM,coord_t>(), \
                      provenance); \
          }
        LEGION_FOREACH_N(DIMFUNC)
#undef DIMFUNC
        default:
          assert(false);
      }
      return IndexSpace::NO_SPACE;
    }

    //--------------------------------------------------------------------------
    IndexSpace MapperRuntime::union_index_spaces(MapperContext ctx,
           const std::vector<IndexSpace> &sources, const char *provenance) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->union_index_spaces(ctx, sources, provenance);
    }

    //--------------------------------------------------------------------------
    IndexSpace MapperRuntime::intersect_index_spaces(MapperContext ctx,
           const std::vector<IndexSpace> &sources, const char *provenance) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->intersect_index_spaces(ctx, sources, provenance);
    }

    //--------------------------------------------------------------------------
    IndexSpace MapperRuntime::subtract_index_spaces(MapperContext ctx,
                IndexSpace left, IndexSpace right, const char *provenance) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->subtract_index_spaces(ctx, left, right, provenance);
    }

    //--------------------------------------------------------------------------
    bool MapperRuntime::is_index_space_empty(MapperContext ctx,
                                             IndexSpace handle) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->is_index_space_empty(ctx, handle);
    }

    //--------------------------------------------------------------------------
    bool MapperRuntime::index_spaces_overlap(MapperContext ctx,
                                           IndexSpace one, IndexSpace two) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->index_spaces_overlap(ctx, one, two);
    }

    //--------------------------------------------------------------------------
    bool MapperRuntime::index_space_dominates(MapperContext ctx,
                                        IndexSpace left, IndexSpace right) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->index_space_dominates(ctx, left, right);
    }

    //--------------------------------------------------------------------------
    bool MapperRuntime::has_index_partition(MapperContext ctx,
                                           IndexSpace parent, Color color) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->has_index_partition(ctx, parent, color);
    }

    //--------------------------------------------------------------------------
    IndexPartition MapperRuntime::get_index_partition(MapperContext ctx,
                                           IndexSpace parent, Color color) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->get_index_partition(ctx, parent, color);
    }

    //--------------------------------------------------------------------------
    IndexSpace MapperRuntime::get_index_subspace(MapperContext ctx, 
                                          IndexPartition p, Color c) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->get_index_subspace(ctx, p, c);
    }

    //--------------------------------------------------------------------------
    IndexSpace MapperRuntime::get_index_subspace(MapperContext ctx, 
                               IndexPartition p, const DomainPoint &color) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->get_index_subspace(ctx, p, color);
    }

    //--------------------------------------------------------------------------
    bool MapperRuntime::has_multiple_domains(MapperContext ctx,
                                                IndexSpace handle) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->has_multiple_domains(ctx, handle);
    }

    //--------------------------------------------------------------------------
    Domain MapperRuntime::get_index_space_domain(MapperContext ctx, 
                                                    IndexSpace handle) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->get_index_space_domain(ctx, handle);
    }

    //--------------------------------------------------------------------------
    void MapperRuntime::get_index_space_domains(MapperContext ctx, 
                          IndexSpace handle, std::vector<Domain> &domains) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->get_index_space_domains(ctx, handle, domains);
    }

    //--------------------------------------------------------------------------
    Domain MapperRuntime::get_index_partition_color_space(MapperContext ctx,
                                                         IndexPartition p) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->get_index_partition_color_space(ctx, p);
    }

    //--------------------------------------------------------------------------
    IndexSpace MapperRuntime::get_index_partition_color_space_name(
                                      MapperContext ctx, IndexPartition p) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->get_index_partition_color_space_name(ctx, p);
    }

    //--------------------------------------------------------------------------
    void MapperRuntime::get_index_space_partition_colors(MapperContext ctx,
                              IndexSpace handle, std::set<Color> &colors) const
    //--------------------------------------------------------------------------
    {
      ctx->manager->get_index_space_partition_colors(ctx, handle, colors);
    }

    //--------------------------------------------------------------------------
    bool MapperRuntime::is_index_partition_disjoint(MapperContext ctx,
                                                       IndexPartition p) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->is_index_partition_disjoint(ctx, p);
    }

    //--------------------------------------------------------------------------
    bool MapperRuntime::is_index_partition_complete(MapperContext ctx,
                                                    IndexPartition p) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->is_index_partition_complete(ctx, p);
    }

    //--------------------------------------------------------------------------
    Color MapperRuntime::get_index_space_color(MapperContext ctx, 
                                                  IndexSpace handle) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->get_index_space_color(ctx, handle);
    }

    //--------------------------------------------------------------------------
    DomainPoint MapperRuntime::get_index_space_color_point(MapperContext ctx, 
                                                        IndexSpace handle) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->get_index_space_color_point(ctx, handle);
    }

    //--------------------------------------------------------------------------
    Color MapperRuntime::get_index_partition_color(MapperContext ctx,
                                                    IndexPartition handle) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->get_index_partition_color(ctx, handle);
    }

    //--------------------------------------------------------------------------
    IndexSpace MapperRuntime::get_parent_index_space(MapperContext ctx,
                                                    IndexPartition handle) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->get_parent_index_space(ctx, handle);
    }

    //--------------------------------------------------------------------------
    bool MapperRuntime::has_parent_index_partition(MapperContext ctx,
                                                      IndexSpace handle) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->has_parent_index_partition(ctx, handle);
    }

    //--------------------------------------------------------------------------
    IndexPartition MapperRuntime::get_parent_index_partition(
                                     MapperContext ctx, IndexSpace handle) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->get_parent_index_partition(ctx, handle);
    }

    //--------------------------------------------------------------------------
    unsigned MapperRuntime::get_index_space_depth(MapperContext ctx,
                                                  IndexSpace handle) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->get_index_space_depth(ctx, handle);
    }

    //--------------------------------------------------------------------------
    unsigned MapperRuntime::get_index_partition_depth(MapperContext ctx,
                                                    IndexPartition handle) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->get_index_partition_depth(ctx, handle);
    }

    //--------------------------------------------------------------------------
    size_t MapperRuntime::get_field_size(MapperContext ctx,
                                           FieldSpace handle, FieldID fid) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->get_field_size(ctx, handle, fid);
    }

    //--------------------------------------------------------------------------
    void MapperRuntime::get_field_space_fields(MapperContext ctx, 
                          FieldSpace handle, std::vector<FieldID> &fields) const
    //--------------------------------------------------------------------------
    {
      ctx->manager->get_field_space_fields(ctx, handle, fields);
    }

    //--------------------------------------------------------------------------
    void MapperRuntime::get_field_space_fields(MapperContext ctx, 
                          FieldSpace handle, std::set<FieldID> &fields) const
    //--------------------------------------------------------------------------
    {
      std::vector<FieldID> local;
      ctx->manager->get_field_space_fields(ctx, handle, local);
      fields.insert(local.begin(), local.end());
    }

    //--------------------------------------------------------------------------
    LogicalPartition MapperRuntime::get_logical_partition(MapperContext ctx,
                              LogicalRegion parent, IndexPartition handle) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->get_logical_partition(ctx, parent, handle);
    }

    //--------------------------------------------------------------------------
    LogicalPartition MapperRuntime::get_logical_partition_by_color(
                        MapperContext ctx, LogicalRegion par, Color color) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->get_logical_partition_by_color(ctx, par, color);
    }

    //--------------------------------------------------------------------------
    LogicalPartition MapperRuntime::get_logical_partition_by_color(
           MapperContext ctx, LogicalRegion par, const DomainPoint &color) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->get_logical_partition_by_color(ctx, par, color);
    }

    //--------------------------------------------------------------------------
    LogicalPartition MapperRuntime::get_logical_partition_by_tree(
                                      MapperContext ctx, IndexPartition part,
                                      FieldSpace fspace, RegionTreeID tid) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->get_logical_partition_by_tree(ctx, part, fspace,tid);
    }

    //--------------------------------------------------------------------------
    LogicalRegion MapperRuntime::get_logical_subregion(MapperContext ctx,
                               LogicalPartition parent, IndexSpace handle) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->get_logical_subregion(ctx, parent, handle);
    }

    //--------------------------------------------------------------------------
    LogicalRegion MapperRuntime::get_logical_subregion_by_color(
                     MapperContext ctx, LogicalPartition par, Color color) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->get_logical_subregion_by_color(ctx, par, color);
    }

    //--------------------------------------------------------------------------
    LogicalRegion MapperRuntime::get_logical_subregion_by_color(
        MapperContext ctx, LogicalPartition par, const DomainPoint &color) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->get_logical_subregion_by_color(ctx, par, color);
    }

    //--------------------------------------------------------------------------
    LogicalRegion MapperRuntime::get_logical_subregion_by_tree(
                                      MapperContext ctx, IndexSpace handle, 
                                      FieldSpace fspace, RegionTreeID tid) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->get_logical_subregion_by_tree(ctx,handle,fspace,tid);
    }

    //--------------------------------------------------------------------------
    Color MapperRuntime::get_logical_region_color(MapperContext ctx,
                                                     LogicalRegion handle) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->get_logical_region_color(ctx, handle);
    }

    //--------------------------------------------------------------------------
    DomainPoint MapperRuntime::get_logical_region_color_point(MapperContext ctx,
                                                     LogicalRegion handle) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->get_logical_region_color_point(ctx, handle);
    }

    //--------------------------------------------------------------------------
    Color MapperRuntime::get_logical_partition_color(MapperContext ctx,
                                                  LogicalPartition handle) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->get_logical_partition_color(ctx, handle);
    }

    //--------------------------------------------------------------------------
    LogicalRegion MapperRuntime::get_parent_logical_region(MapperContext ctx,
                                                    LogicalPartition part) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->get_parent_logical_region(ctx, part);
    }
    
    //--------------------------------------------------------------------------
    bool MapperRuntime::has_parent_logical_partition(MapperContext ctx,
                                                     LogicalRegion handle) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->has_parent_logical_partition(ctx, handle);
    }

    //--------------------------------------------------------------------------
    LogicalPartition MapperRuntime::get_parent_logical_partition(
                                       MapperContext ctx, LogicalRegion r) const
    //--------------------------------------------------------------------------
    {
      return ctx->manager->get_parent_logical_partition(ctx, r);
    }

    //--------------------------------------------------------------------------
    bool MapperRuntime::retrieve_semantic_information(MapperContext ctx,
        TaskID task_id, SemanticTag tag, const void *&result, size_t &size,
        bool can_fail, bool wait_until_ready)
    //--------------------------------------------------------------------------
    {
      return ctx->manager->retrieve_semantic_information(ctx, task_id,
					  tag, result,
                                          size, can_fail, wait_until_ready);
    }

    //--------------------------------------------------------------------------
    bool MapperRuntime::retrieve_semantic_information(MapperContext ctx, 
          IndexSpace handle, SemanticTag tag, const void *&result, size_t &size,
          bool can_fail, bool wait_until_ready)
    //--------------------------------------------------------------------------
    {
      return ctx->manager->retrieve_semantic_information(ctx, handle,
					  tag, result,
                                          size, can_fail, wait_until_ready);
    }

    //--------------------------------------------------------------------------
    bool MapperRuntime::retrieve_semantic_information(MapperContext ctx,
          IndexPartition handle, SemanticTag tag, const void *&result, 
          size_t &size, bool can_fail, bool wait_until_ready)
    //--------------------------------------------------------------------------
    {
      return ctx->manager->retrieve_semantic_information(ctx, handle,
					  tag, result,
                                          size, can_fail, wait_until_ready);
    }

    //--------------------------------------------------------------------------
    bool MapperRuntime::retrieve_semantic_information(MapperContext ctx,
          FieldSpace handle, SemanticTag tag, const void *&result, size_t &size,
          bool can_fail, bool wait_until_ready)
    //--------------------------------------------------------------------------
    {
      return ctx->manager->retrieve_semantic_information(ctx, handle,
					  tag, result,
                                          size, can_fail, wait_until_ready);
    }

    //--------------------------------------------------------------------------
    bool MapperRuntime::retrieve_semantic_information(MapperContext ctx, 
          FieldSpace handle, FieldID fid, SemanticTag tag, const void *&result, 
          size_t &size, bool can_fail, bool wait_until_ready)
    //--------------------------------------------------------------------------
    {
      return ctx->manager->retrieve_semantic_information(ctx, handle, fid, tag,
					  result, size,
                                          can_fail, wait_until_ready);
    }

    //--------------------------------------------------------------------------
    bool MapperRuntime::retrieve_semantic_information(MapperContext ctx,
          LogicalRegion handle, SemanticTag tag, const void *&result, 
          size_t &size, bool can_fail, bool wait_until_ready)
    //--------------------------------------------------------------------------
    {
      return ctx->manager->retrieve_semantic_information(ctx, handle, tag,
					  result, size,
                                          can_fail, wait_until_ready);
    }

    //--------------------------------------------------------------------------
   bool MapperRuntime::retrieve_semantic_information(MapperContext ctx,
          LogicalPartition handle, SemanticTag tag, const void *&result, 
          size_t &size, bool can_fail, bool wait_until_ready)
    //--------------------------------------------------------------------------
    {
      return ctx->manager->retrieve_semantic_information(ctx, handle, tag,
					  result, size,
                                          can_fail, wait_until_ready);
    }

    //--------------------------------------------------------------------------
    void MapperRuntime::retrieve_name(MapperContext ctx, TaskID task_id,
                                         const char *&result)
    //--------------------------------------------------------------------------
    {
      ctx->manager->retrieve_name(ctx, task_id, result);
    }

    //--------------------------------------------------------------------------
    void MapperRuntime::retrieve_name(MapperContext ctx, IndexSpace handle,
                                         const char *&result)
    //--------------------------------------------------------------------------
    {
      ctx->manager->retrieve_name(ctx, handle, result);
    }

    //--------------------------------------------------------------------------
    void MapperRuntime::retrieve_name(MapperContext ctx, 
                                     IndexPartition handle, const char *&result)
    //--------------------------------------------------------------------------
    {
      ctx->manager->retrieve_name(ctx, handle, result);
    }

    //--------------------------------------------------------------------------
    void MapperRuntime::retrieve_name(MapperContext ctx,
                                         FieldSpace handle, const char *&result)
    //--------------------------------------------------------------------------
    {
      ctx->manager->retrieve_name(ctx, handle, result);
    }

    //--------------------------------------------------------------------------
    void MapperRuntime::retrieve_name(MapperContext ctx, FieldSpace handle,
                                         FieldID fid, const char *&result)
    //--------------------------------------------------------------------------
    {
      ctx->manager->retrieve_name(ctx, handle, fid, result);
    }

    //--------------------------------------------------------------------------
    void MapperRuntime::retrieve_name(MapperContext ctx, 
                                      LogicalRegion handle, const char *&result)
    //--------------------------------------------------------------------------
    {
      ctx->manager->retrieve_name(ctx, handle, result);
    }

    //--------------------------------------------------------------------------
    void MapperRuntime::retrieve_name(MapperContext ctx,
                                   LogicalPartition handle, const char *&result)
    //--------------------------------------------------------------------------
    {
      ctx->manager->retrieve_name(ctx, handle, result);
    }

    //--------------------------------------------------------------------------
    bool MapperRuntime::is_MPI_interop_configured(MapperContext ctx)
    //--------------------------------------------------------------------------
    {
      return ctx->manager->is_MPI_interop_configured();
    }

    //--------------------------------------------------------------------------
    const std::map<int,AddressSpace>& MapperRuntime::find_forward_MPI_mapping(
                                                              MapperContext ctx)
    //--------------------------------------------------------------------------
    {
      return ctx->manager->find_forward_MPI_mapping(ctx);
    }

    //--------------------------------------------------------------------------
    const std::map<AddressSpace,int>& MapperRuntime::find_reverse_MPI_mapping(
                                                              MapperContext ctx)
    //--------------------------------------------------------------------------
    {
      return ctx->manager->find_reverse_MPI_mapping(ctx);
    }

    //--------------------------------------------------------------------------
    int MapperRuntime::find_local_MPI_rank(MapperContext ctx)
    //--------------------------------------------------------------------------
    {
      return ctx->manager->find_local_MPI_rank();
    }

    /////////////////////////////////////////////////////////////
    // AutoLock
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    AutoLock::AutoLock(MapperContext c, LocalLock &r, int mode, bool excl)
      : Internal::AutoLock(mode, excl, r), ctx(c)
    //--------------------------------------------------------------------------
    {
      bool paused = false;
      if (exclusive)
      {
        Internal::RtEvent ready = local_lock.wrlock();
        while (ready.exists())
        {
          if (!paused)
          {
            ctx->manager->pause_mapper_call(ctx);
            paused = true;
          }
          ready.wait();
          ready = local_lock.wrlock();
        }
      }
      else
      {
        Internal::RtEvent ready = local_lock.rdlock();
        while (ready.exists())
        {
          if (!paused)
          {
            ctx->manager->pause_mapper_call(ctx);
            paused = true;
          }
          ready.wait();
          ready = local_lock.rdlock();
        }
      }
      held = true;
      Internal::local_lock_list = this;
      if (paused)
        ctx->manager->resume_mapper_call(ctx, Internal::MAPPER_AUTO_LOCK_CALL);
    }

    //--------------------------------------------------------------------------
    void AutoLock::reacquire(void)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(!held);
      assert(Internal::local_lock_list == previous);
#endif
#ifdef DEBUG_REENTRANT_LOCKS
      if (previous != NULL)
        previous->check_for_reentrant_locks(&local_lock);
#endif
      bool paused = false;
      if (exclusive)
      {
        Internal::RtEvent ready = local_lock.wrlock();
        while (ready.exists())
        {
          if (!paused)
          {
            ctx->manager->pause_mapper_call(ctx);
            paused = true;
          }
          ready.wait();
          ready = local_lock.wrlock();
        }
      }
      else
      {
        Internal::RtEvent ready = local_lock.rdlock();
        while (ready.exists())
        {
          if (!paused)
          {
            ctx->manager->pause_mapper_call(ctx);
            paused = true;
          }
          ready.wait();
          ready = local_lock.rdlock();
        }
      }
      Internal::local_lock_list = this;
      held = true;
      if (paused)
        ctx->manager->resume_mapper_call(ctx, Internal::MAPPER_AUTO_LOCK_CALL);
    }

  }; // namespace Mapping
}; // namespace Legion

