#include <chrono>
#include <memory>
#include <utility>
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_components/register_node_macro.hpp"
#include "std_msgs/msg/string.hpp"

namespace demo_component {
class Talker : public rclcpp::Node {
public:
  explicit Talker(const rclcpp::NodeOptions & options)
  : Node("component_talker", options) {
    publisher_ = create_publisher<std_msgs::msg::String>("component_chatter", 10);
    timer_ = create_wall_timer(std::chrono::seconds(1), [this]() {
      auto message = std::make_unique<std_msgs::msg::String>();
      message->data = "component message " + std::to_string(count_++);
      RCLCPP_INFO(get_logger(), "PUB ptr=%p data=%s", static_cast<void *>(message.get()), message->data.c_str());
      publisher_->publish(std::move(message));
    });
  }
private:
  size_t count_{0};
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
  rclcpp::TimerBase::SharedPtr timer_;
};
class Listener : public rclcpp::Node {
public:
  explicit Listener(const rclcpp::NodeOptions & options)
  : Node("component_listener", options) {
    subscription_ = create_subscription<std_msgs::msg::String>("component_chatter", 10,
      [this](std_msgs::msg::String::UniquePtr message) {
        RCLCPP_INFO(get_logger(), "SUB ptr=%p data=%s", static_cast<void *>(message.get()), message->data.c_str());
      });
  }
private:
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr subscription_;
};
}
RCLCPP_COMPONENTS_REGISTER_NODE(demo_component::Talker)
RCLCPP_COMPONENTS_REGISTER_NODE(demo_component::Listener)
