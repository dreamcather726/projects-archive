from managed_monitor_py.anomaly_detector import AnomalyDetector


def test_safe_distance_has_no_alarm():
    detector = AnomalyDetector(0.6)
    assert detector.evaluate(0.6) is None


def test_close_obstacle_has_alarm():
    detector = AnomalyDetector(0.6)
    assert '障碍物过近' in detector.evaluate(0.59)


def test_invalid_sensor_value_has_alarm():
    detector = AnomalyDetector(0.6)
    assert detector.evaluate(-1.0) == '传感器数据非法'
