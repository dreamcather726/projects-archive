"""动画 GIF 录制器。"""

try:
    import imageio
except ModuleNotFoundError as module_not_found:
    raise RuntimeError(
        '未找到包: imageio。使用 `pip install imageio` 安装。') from module_not_found

import gym

__all__ = ('GIFRecorder')


class GIFRecorder(gym.Wrapper):
    """动画 GIF 录制器。"""

    def __init__(self, env, filename_template, use_seed=False):
        """AnimRecorder 构造函数。

        参数:
            env {gym.Env} -- 要包装的环境
            filename_template {str} -- GIF 文件名模板（如 'record_{:04d}.gif'）。

        关键字参数:
            use_seed {bool} -- 使用种子作为文件 ID（在模板中替换）(默认: {False})。
        """
        super().__init__(env)
        self.env.unwrapped.show_window(False)
        self._filename_template = filename_template
        self._use_seed = use_seed

        input_fps = self.env.unwrapped.metadata.get('video.frames_per_second', 30.0)
        output_fps = self.env.unwrapped.metadata.get('video.output_frames_per_second', 30.0)
        self._frames_per_second = output_fps
        self._steps_per_frame = max(1, input_fps // output_fps)

        self._reset_counter = 0
        self._step_counter = 0
        self._writer = None

    def reset(self, **kwargs):
        """重置环境到初始状态。

        打开 GIF 文件写入器。
        """
        if self._writer is not None:
            self._writer.close()

        uid = self._seed if self._use_seed else self._reset_counter
        filename = self._filename_template.format(uid)
        self._writer = imageio.get_writer(filename, mode='I', fps=self._frames_per_second)

        self._reset_counter += 1
        self._step_counter = 0
        return self.env.reset(**kwargs)

    def step(self, action):
        """运行环境动力学的一个时间步。

        渲染图像并写入文件。
        """
        if self._step_counter % self._steps_per_frame == 0:
            image_rgb = self.env.render('rgb_array')
            self._writer.append_data(image_rgb)
        self._step_counter += 1
        return self.env.step(action)

    def close(self):
        """关闭环境。

        将录制的数据刷新到文件。
        """
        if self._writer is not None:
            self._writer.close()
        return self.env.close()
