﻿// ----------------------------------------------------------------------------
// -                        Open3D: www.open3d.org                            -
// ----------------------------------------------------------------------------
// Copyright (c) 2018-2023 www.open3d.org
// SPDX-License-Identifier: MIT
// ----------------------------------------------------------------------------

#include <numeric>
#include <unordered_map>

#include "open3d/geometry/IntersectionTest.h"
#include "open3d/geometry/PointCloud.h"
#include "open3d/geometry/TriangleMesh.h"
#include "open3d/geometry/VoxelGrid.h"
#include "open3d/utility/Helper.h"
#include "open3d/utility/Logging.h"

namespace open3d {
	namespace geometry {
		std::shared_ptr<VoxelGrid> VoxelGrid::CreateDense(const Eigen::Vector3d& origin,
			const Eigen::Vector3d& color,
			double voxel_size,
			double width,
			double height,
			double depth) {
			auto output = std::make_shared<VoxelGrid>();
			int num_w = int(std::round(width / voxel_size));
			int num_h = int(std::round(height / voxel_size));
			int num_d = int(std::round(depth / voxel_size));
			output->origin_ = origin;
			output->voxel_size_ = voxel_size;
			for (int widx = 0; widx < num_w; widx++) {
				for (int hidx = 0; hidx < num_h; hidx++) {
					for (int didx = 0; didx < num_d; didx++) {
						Eigen::Vector3i grid_index(widx, hidx, didx);
						output->AddVoxel(geometry::Voxel(grid_index, color));
					}
				}
			}
			return output;
		}

		std::shared_ptr<VoxelGrid> VoxelGrid::CreateFromPointCloudWithinBounds(
			const PointCloud& input,
			double voxel_size,
			const Eigen::Vector3d& min_bound,
			const Eigen::Vector3d& max_bound) {
			auto output = std::make_shared<VoxelGrid>();
			if (voxel_size <= 0.0) {
				utility::LogError("voxel_size <= 0.");
			}

			if (voxel_size * std::numeric_limits<int>::max() <
				(max_bound - min_bound).maxCoeff()) {
				utility::LogError("voxel_size is too small.");
			}
			output->voxel_size_ = voxel_size;
			output->origin_ = min_bound;
			std::unordered_map<Eigen::Vector3i, AvgColorVoxel,
				utility::hash_eigen<Eigen::Vector3i>>
				voxelindex_to_accpoint;
			Eigen::Vector3d ref_coord;
			Eigen::Vector3i voxel_index;
			bool has_colors = input.HasColors();
			for (int i = 0; i < (int)input.points_.size(); i++) {
				ref_coord = (input.points_[i] - min_bound) / voxel_size;
				voxel_index << int(floor(ref_coord(0))), int(floor(ref_coord(1))),
					int(floor(ref_coord(2)));
				if (has_colors) {
					voxelindex_to_accpoint[voxel_index].Add(voxel_index,
						input.colors_[i]);
				}
				else {
					voxelindex_to_accpoint[voxel_index].Add(voxel_index);
				}
			}
			for (auto accpoint : voxelindex_to_accpoint) {
				const Eigen::Vector3i& grid_index = accpoint.second.GetVoxelIndex();
				const Eigen::Vector3d& color =
					has_colors ? accpoint.second.GetAverageColor()
					: Eigen::Vector3d(0, 0, 0);
				output->AddVoxel(geometry::Voxel(grid_index, color));
			}
			utility::LogDebug(
				"Pointcloud is voxelized from {:d} points to {:d} voxels.",
				(int)input.points_.size(), (int)output->voxels_.size());
			return output;
		}

		std::shared_ptr<VoxelGrid> VoxelGrid::CreateFromPointCloud(
			const PointCloud& input, double voxel_size) {
			Eigen::Vector3d voxel_size3(voxel_size, voxel_size, voxel_size);
			Eigen::Vector3d min_bound = input.GetMinBound() - voxel_size3 * 0.5;
			Eigen::Vector3d max_bound = input.GetMaxBound() + voxel_size3 * 0.5;
			return CreateFromPointCloudWithinBounds(input, voxel_size, min_bound,
				max_bound);
		}
	}  // namespace geometry
}  // namespace open3d