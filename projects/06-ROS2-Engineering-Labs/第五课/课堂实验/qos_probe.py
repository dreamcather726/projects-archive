"""QoS observations with sequence, sample age and optional deadline events."""
import argparse
import time
import rclpy
from rclpy.node import Node
from rclpy.duration import Duration
from rclpy.qos import QoSProfile, ReliabilityPolicy, DurabilityPolicy, HistoryPolicy
from rclpy.qos_event import PublisherEventCallbacks, SubscriptionEventCallbacks
from std_msgs.msg import String


class Probe(Node):
    def __init__(self, args):
        super().__init__('qos_' + args.role)
        self.args = args
        self.sequence = 0
        profile = QoSProfile(
            history=HistoryPolicy.KEEP_LAST, depth=args.depth,
            reliability=(ReliabilityPolicy.RELIABLE if args.reliability == 'reliable'
                         else ReliabilityPolicy.BEST_EFFORT),
            durability=(DurabilityPolicy.TRANSIENT_LOCAL if args.durability == 'transient_local'
                        else DurabilityPolicy.VOLATILE))
        if args.deadline > 0:
            profile.deadline = Duration(seconds=args.deadline)
        if args.lifespan > 0:
            profile.lifespan = Duration(seconds=args.lifespan)
        if args.role == 'pub':
            options = {}
            if args.deadline > 0:
                options['event_callbacks'] = PublisherEventCallbacks(deadline=self.missed_deadline)
            self.publisher = self.create_publisher(String, '/qos/probe', profile, **options)
            self.timer = self.create_timer(args.period, self.send)
        else:
            options = {}
            if args.deadline > 0:
                options['event_callbacks'] = SubscriptionEventCallbacks(deadline=self.missed_deadline)
            self.subscription = self.create_subscription(String, '/qos/probe', self.receive,
                                                         profile, **options)

    def send(self):
        message = String()
        message.data = f'{self.sequence} {time.monotonic():.6f}'
        self.publisher.publish(message)
        self.get_logger().info('SEND ' + message.data)
        self.sequence += 1
        if self.args.count and self.sequence >= self.args.count:
            self.timer.cancel()
            self.get_logger().info('Publishing paused; node stays alive for late subscribers')

    def receive(self, message):
        sequence, created = message.data.split()
        age = time.monotonic() - float(created)
        self.get_logger().info(f'RECV seq={sequence} age={age:.3f}s')
        if self.args.slow:
            time.sleep(self.args.slow)

    def missed_deadline(self, event):
        self.get_logger().warning(f'DEADLINE total={event.total_count} change={event.total_count_change}')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('role', choices=['pub', 'sub'])
    parser.add_argument('--reliability', choices=['reliable', 'best_effort'], default='reliable')
    parser.add_argument('--durability', choices=['volatile', 'transient_local'], default='volatile')
    parser.add_argument('--depth', type=int, default=5)
    parser.add_argument('--period', type=float, default=0.1)
    parser.add_argument('--slow', type=float, default=0.0)
    parser.add_argument('--count', type=int, default=0)
    parser.add_argument('--deadline', type=float, default=0.0)
    parser.add_argument('--lifespan', type=float, default=0.0)
    args, ros_args = parser.parse_known_args()
    if args.depth < 1 or args.period <= 0 or min(args.slow, args.count, args.deadline, args.lifespan) < 0:
        parser.error('depth and period must be positive; other numeric options nonnegative')
    rclpy.init(args=ros_args)
    node = None
    try:
        node = Probe(args)
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        if node is not None:
            node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == '__main__':
    main()
