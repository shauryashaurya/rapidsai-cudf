/*
 * Copyright (c) 2020-2024, NVIDIA CORPORATION.
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

#pragma once

#include <cudf/types.hpp>
#include <cudf/utilities/default_stream.hpp>

#include <rmm/cuda_stream_view.hpp>
#include <rmm/resource_ref.hpp>

#include <memory>

namespace CUDF_EXPORT cudf {
namespace detail {
/**
 * @copydoc cudf::tile
 *
 * @param stream CUDA stream used for device memory operations and kernel launches
 */
std::unique_ptr<table> tile(table_view const& input,
                            size_type count,
                            rmm::cuda_stream_view,
                            rmm::device_async_resource_ref mr);

/**
 * @copydoc cudf::interleave_columns
 *
 * @param stream CUDA stream used for device memory operations and kernel launches
 */
std::unique_ptr<column> interleave_columns(table_view const& input,
                                           rmm::cuda_stream_view,
                                           rmm::device_async_resource_ref mr);

}  // namespace detail
}  // namespace CUDF_EXPORT cudf
