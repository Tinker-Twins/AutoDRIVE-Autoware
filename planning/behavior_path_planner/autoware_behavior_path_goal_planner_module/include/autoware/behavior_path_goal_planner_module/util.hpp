// Copyright 2021 Tier IV, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef AUTOWARE__BEHAVIOR_PATH_GOAL_PLANNER_MODULE__UTIL_HPP_
#define AUTOWARE__BEHAVIOR_PATH_GOAL_PLANNER_MODULE__UTIL_HPP_

#include "autoware/behavior_path_goal_planner_module/goal_candidate.hpp"
#include "autoware/behavior_path_goal_planner_module/pull_over_planner/pull_over_planner_base.hpp"
#include "autoware_utils/geometry/boost_geometry.hpp"

#include <autoware/boundary_departure_checker/boundary_departure_checker.hpp>

#include <autoware_internal_planning_msgs/msg/path_with_lane_id.hpp>
#include <autoware_perception_msgs/msg/predicted_objects.hpp>
#include <autoware_perception_msgs/msg/predicted_path.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <geometry_msgs/msg/twist_stamped.hpp>
#include <visualization_msgs/msg/marker_array.hpp>

#include <lanelet2_core/Forward.h>

#include <limits>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace autoware::behavior_path_planner::goal_planner_utils
{
using autoware_internal_planning_msgs::msg::PathWithLaneId;
using autoware_perception_msgs::msg::PredictedObjects;
using autoware_perception_msgs::msg::PredictedPath;
using geometry_msgs::msg::Pose;
using geometry_msgs::msg::Twist;
using visualization_msgs::msg::Marker;
using visualization_msgs::msg::MarkerArray;
using Shape = autoware_perception_msgs::msg::Shape;
using Polygon2d = autoware_utils::Polygon2d;
using autoware_utils::LineString2d;
using autoware_utils::Point2d;
using autoware_utils::Segment2d;

using SegmentRtree = boost::geometry::index::rtree<Segment2d, boost::geometry::index::rstar<16>>;

lanelet::BoundingBox2d polygon_to_boundingbox(const Polygon2d & polygon);

SegmentRtree extract_uncrossable_segments(
  const lanelet::LaneletMap & lanelet_map, const Polygon2d & extraction_polygon);

void add_intersecting_segments(
  const lanelet::ConstLineString3d & ls, const Polygon2d & extraction_polygon,
  SegmentRtree & segments_rtree);

bool has_types(const lanelet::ConstLineString3d & ls, const std::vector<std::string> & types);

PredictedObjects filter_objects_by_road_border(
  const PredictedObjects & objects, const SegmentRtree & road_border_segments,
  const Pose & ego_pose, const bool filter_opposite_side);

bool crosses_road_border(
  const Point2d & ego_point, const Point2d & obj_point, const SegmentRtree & road_border_segments);

lanelet::ConstLanelets getPullOverLanes(
  const RouteHandler & route_handler, const bool left_side, const double backward_distance,
  const double forward_distance);

lanelet::ConstLanelets generateBetweenEgoAndExpandedPullOverLanes(
  const lanelet::ConstLanelets & pull_over_lanes, const bool left_side,
  const geometry_msgs::msg::Pose ego_pose,
  const autoware::vehicle_info_utils::VehicleInfo & vehicle_info, const double outer_road_offset,
  const double inner_road_offset);

/*
 * @brief generate polygon to extract objects
 * @param pull_over_lanes pull over lanes
 * @param left_side left side or right side
 * @param outer_offset outer offset from pull over lane boundary
 * @param inner_offset inner offset from pull over lane boundary
 * @return polygon to extract objects
 */
std::optional<Polygon2d> generateObjectExtractionPolygon(
  const lanelet::ConstLanelets & pull_over_lanes, const bool left_side, const double outer_offset,
  const double inner_offset);

PredictedObjects filterObjectsByLateralDistance(
  const Pose & ego_pose, const double vehicle_width, const PredictedObjects & objects,
  const double distance_thresh, const bool filter_inside);

double calcLateralDeviationBetweenPaths(
  const PathWithLaneId & reference_path, const PathWithLaneId & target_path);
bool isReferencePath(
  const PathWithLaneId & reference_path, const PathWithLaneId & target_path,
  const double lateral_deviation_thresh);

std::optional<PathWithLaneId> cropPath(const PathWithLaneId & path, const Pose & end_pose);
PathWithLaneId cropForwardPoints(
  const PathWithLaneId & path, const size_t target_seg_idx, const double forward_length);

/**
 * @brief extend target_path by extend_length
 * @param target_path original target path to extend
 * @param reference_path reference path to extend
 * @param extend_length length to extend
 * @param remove_connected_zero_velocity flag to remove zero velocity if the last point of
 *                                       target_path has zero velocity
 * @return extended path
 */
PathWithLaneId extendPath(
  const PathWithLaneId & target_path, const PathWithLaneId & reference_path,
  const double extend_length, const bool remove_connected_zero_velocity);
/**
 * @brief extend target_path to extend_pose
 * @param target_path original target path to extend
 * @param reference_path reference path to extend
 * @param extend_pose pose to extend
 * @param remove_connected_zero_velocity flag to remove zero velocity if the last point of
 *                                       target_path has zero velocity
 * @return extended path
 */
PathWithLaneId extendPath(
  const PathWithLaneId & target_path, const PathWithLaneId & reference_path,
  const Pose & extend_pose, const bool remove_connected_zero_velocity);

std::vector<Polygon2d> createPathFootPrints(
  const PathWithLaneId & path, const double base_to_front, const double base_to_rear,
  const double width);

/**
 * @brief check if footprint intersects with given areas
 */
bool isIntersectingAreas(
  const LinearRing2d & footprint, const std::vector<lanelet::BasicPolygon2d> & areas);

/**
 * @brief check if footprint is within one of the areas
 */
bool isWithinAreas(
  const LinearRing2d & footprint, const std::vector<lanelet::BasicPolygon2d> & areas);

/**
 * @brief query BusStopArea polygons associated with given lanes
 */
std::vector<lanelet::BasicPolygon2d> getBusStopAreaPolygons(const lanelet::ConstLanelets & lanes);

/**
 * @brief check collision between objects and ego path footprints
 * @param path ego path to check collision
 * @param curvatures curvatures of ego path
 * @param static_target_objects static objects to check collision
 * @param dynamic_target_objects dynamic objects to check collision
 * @param behavior_path_parameters behavior path parameters
 * @param collision_check_margin margin to check collision
 * @param extract_static_objects flag to extract static objects
 * @param maximum_deceleration maximum deceleration
 * @param object_recognition_collision_check_max_extra_stopping_margin maximum extra stopping margin
 * @param collision_check_outer_margin_factor factor to extend the collision check margin from the
 *                                            inside margin to the outside in the curved path
 * @param ego_polygons_expanded expanded ego polygons
 * @param update_debug_data flag to update debug data
 * @return true if collision is detected
 */
bool checkObjectsCollision(
  const PathWithLaneId & path, const std::vector<double> & curvatures,
  const PredictedObjects & static_target_objects, const PredictedObjects & dynamic_target_objects,
  const BehaviorPathPlannerParameters & behavior_path_parameters,
  const double collision_check_margin, const bool extract_static_objects,
  const double maximum_deceleration,
  const double object_recognition_collision_check_max_extra_stopping_margin,
  const double collision_check_outer_margin_factor, std::vector<Polygon2d> & ego_polygons_expanded,
  const bool update_debug_data = false);

// debug
MarkerArray createPullOverAreaMarkerArray(
  const autoware_utils::MultiPolygon2d area_polygons, const std_msgs::msg::Header & header,
  const std_msgs::msg::ColorRGBA & color, const double z);
MarkerArray createPosesMarkerArray(
  const std::vector<Pose> & poses, std::string && ns, const std_msgs::msg::ColorRGBA & color);
MarkerArray createTextsMarkerArray(
  const std::vector<Pose> & poses, std::string && ns, const std_msgs::msg::ColorRGBA & color);
std::pair<MarkerArray, MarkerArray> createGoalCandidatesMarkerArray(
  const GoalCandidates & goal_candidates, const std_msgs::msg::ColorRGBA & color);
MarkerArray createLaneletPolygonMarkerArray(
  const lanelet::CompoundPolygon3d & polygon, const std_msgs::msg::Header & header,
  const std::string & ns, const std_msgs::msg::ColorRGBA & color);
MarkerArray createNumObjectsToAvoidTextsMarkerArray(
  const GoalCandidates & goal_candidates, std::string && ns,
  const std_msgs::msg::ColorRGBA & color);
std::string makePathPriorityDebugMessage(
  const std::vector<size_t> & sorted_path_indices,
  const std::vector<PullOverPath> & pull_over_path_candidates,
  const std::map<size_t, size_t> & goal_id_to_index, const GoalCandidates & goal_candidates,
  const std::map<size_t, double> & path_id_to_rough_margin_map,
  const std::function<bool(const PullOverPath &)> & isSoftMargin,
  const std::function<bool(const PullOverPath &)> & isHighCurvature);
/**
 * @brief combine two points
 * @param points lane points
 * @param points_next next lane points
 * @return combined points
 */
lanelet::Points3d combineLanePoints(
  const lanelet::Points3d & points, const lanelet::Points3d & points_next);
/** @brief Create a lanelet that represents the departure check area.
 * @param [in] pull_over_lanes Lanelets that the vehicle will pull over to.
 * @param [in] route_handler RouteHandler object.
 * @return Lanelet that goal footprints should be inside.
 */
lanelet::Lanelet createDepartureCheckLanelet(
  const lanelet::ConstLanelets & pull_over_lanes, const route_handler::RouteHandler & route_handler,
  const bool left_side_parking);

std::optional<Pose> calcRefinedGoal(
  const Pose & goal_pose, const std::shared_ptr<RouteHandler> route_handler,
  const bool left_side_parking, const double vehicle_width, const double base_link2front,
  const double base_link2rear, const GoalPlannerParameters & parameters);

std::optional<Pose> calcClosestPose(
  const lanelet::ConstLineString3d line, const Point & query_point);

autoware_perception_msgs::msg::PredictedObjects extract_dynamic_objects(
  const autoware_perception_msgs::msg::PredictedObjects & original_objects,
  const route_handler::RouteHandler & route_handler, const GoalPlannerParameters & parameters,
  const double vehicle_width, const Pose & ego_pose,
  std::optional<std::reference_wrapper<Polygon2d>> debug_objects_extraction_polygon = std::nullopt);

bool is_goal_reachable_on_path(
  const lanelet::ConstLanelets current_lanes, const route_handler::RouteHandler & route_handler,
  const bool left_side_parking);

bool hasPreviousModulePathShapeChanged(
  const BehaviorModuleOutput & upstream_module_output,
  const BehaviorModuleOutput & last_upstream_module_output);
bool hasDeviatedFromPath(
  const Point & ego_position, const BehaviorModuleOutput & upstream_module_output);

/**
 * @brief check if stopline exists except for the terminal
 * @note except for terminal, to account for lane change bug that inserts stopline at the end
 * randomly
 */
bool has_stopline_except_terminal(const PathWithLaneId & path);

/**
 * @brief find the lanelet that has changed "laterally" from previous lanelet on the routing graph
 * @return the lanelet that changed "laterally" if the path is lane changing, otherwise nullopt
 */
std::optional<lanelet::ConstLanelet> find_lane_change_completed_lanelet(
  const PathWithLaneId & path, const lanelet::LaneletMapConstPtr lanelet_map,
  const lanelet::routing::RoutingGraphConstPtr routing_graph);

/**
 * @brief generate lanelets with which pull over path is aligned
 * @note if lane changing path is detected, this returns lanelets aligned with later part of the
 * lane changing path
 */
lanelet::ConstLanelets get_reference_lanelets_for_pullover(
  const PathWithLaneId & path, const std::shared_ptr<const PlannerData> & planner_data,
  const double backward_length, const double forward_length);

}  // namespace autoware::behavior_path_planner::goal_planner_utils

#endif  // AUTOWARE__BEHAVIOR_PATH_GOAL_PLANNER_MODULE__UTIL_HPP_
