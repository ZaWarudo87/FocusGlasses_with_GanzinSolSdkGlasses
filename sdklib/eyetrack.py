from collections import deque
import numpy as np

class EyeTrackingSystem:
    def __init__(self, window_size=300):
        self.gaze_x_deque = deque(maxlen=window_size)
        self.gaze_y_deque = deque(maxlen=window_size)
        self.timestamp_deque = deque(maxlen=window_size)
        self.gaze_data_history = np.zeros((1328, 1200), dtype=np.int64)
        self.dwell_time = 0
        self.status = 'none' # fixation, saccade, none
    
    def add_data(self, gaze_x, gaze_y, timestamp):
        self.gaze_x_deque.append(gaze_x)
        self.gaze_y_deque.append(gaze_y)
        self.timestamp_deque.append(timestamp * 1000)
          
    def fixation_detection(self, window_size = 15):
        x = int(self.gaze_x_deque[-1])
        y = int(self.gaze_y_deque[-1])
        timestamp = self.timestamp_deque[-1]
        if x < 0 or x >= 1328 or y < 0 or y >= 1200:
            return
        
        window_slice = self.gaze_data_history[x - window_size:x + window_size + 1, y - window_size:y + window_size + 1]
        if np.any(window_slice):
            self.dwell_time = timestamp - np.min(window_slice[window_slice> 0])
            if (self.dwell_time > 500):
                self.status = 'fixation'
            else:
                self.status = 'saccade'
        else:          
            self.status = 'saccade'
            self.dwell_time = 0
            self.gaze_data_history.fill(0)
        
        self.gaze_data_history[x - window_size:x + window_size + 1, y - window_size:y + window_size + 1] = timestamp - self.dwell_time
        return self.status