from ganzin.sol_sdk.streaming.gaze_stream import GazeData
from ganzin.sol_sdk.synchronous.models import StreamingMode
from ganzin.sol_sdk.synchronous.sync_client import SyncClient
from ganzin.sol_sdk.utils import find_nearest_timestamp_match
from sdklib import eyetrack as ETS
from flask import Flask, jsonify
from flask_cors import CORS
import cv2
import numpy as np
import threading
import time

app = Flask(__name__)
CORS(app)

now_data = {
    "now" : -1,
    "nowObject" : None,
    "nowEyetrack" : "none",
    "preObject" : None,
    "preEyetrack" : "none",
    "preObjectStart" : -1,
    "studyStartTime" : -1,
    "studyTime" : -1,
    "distractingStartTime" : -1,
    "studyDurationList" : [],
    "maxStudyDuration" : 0,
    "distractionsInterval" : 0,
    "distractionsTimePerTenMin" : [],
    "overTenSec" : False,
    "overTwentySec" : False,
    "preEyetrackStart" : -1
}

@app.route('/')
def update_data():
    return jsonify(now_data)

def detect():
    # import yolo
    version = "v3"
    net = cv2.dnn.readNet(f"yolo{version}/yolo{version}.weights", f"yolo{version}/yolo{version}.cfg")
    layer_names = net.getLayerNames()
    output_layers = [layer_names[i - 1] for i in net.getUnconnectedOutLayers()]

    # read coco
    with open(f"yolo{version}/coco.names", "r") as f:
        classes = [line.strip() for line in f.readlines()]

    # GPU
    net.setPreferableBackend(cv2.dnn.DNN_BACKEND_CUDA)
    net.setPreferableTarget(cv2.dnn.DNN_TARGET_CUDA)

    address = 'Sol SDK IP'
    port = '8080'
    sc = SyncClient(address, port)

    ets = ETS.EyeTrackingSystem()

    th = sc.create_streaming_thread(StreamingMode.GAZE_WORLD)
    th.start()

    frame_data = sc.get_world_frames_from_streaming(timeout=5.0)
    frame_datum = frame_data[-1] # get the last frame
    buffer = frame_datum.get_buffer()
    buffer = cv2.resize(buffer, None, fx=0.5, fy=0.5, interpolation=cv2.INTER_AREA)
    height, width, channels = buffer.shape

    try:
        while True:
            now_data["now"] = int(time.time())
            now_data["preObject"] = now_data["nowObject"]
            now_data["preEyetrack"] = now_data["nowEyetrack"]

            try:
                frame_data = sc.get_world_frames_from_streaming(timeout=5.0)
                frame_datum = frame_data[-1] # get the last frame
                buffer = frame_datum.get_buffer()

                gazes = sc.get_gazes_from_streaming(timeout=5.0)
                gaze = find_nearest_timestamp_match(frame_datum.get_timestamp(), gazes)
                ets.add_data(gaze.combined.gaze_2d.x, gaze.combined.gaze_2d.y, gaze.timestamp)
                now_data["nowEyetrack"] = ets.fixation_detection()
                
                buffer = cv2.resize(buffer, None, fx=0.5, fy=0.5, interpolation=cv2.INTER_AREA)
                center = (int(gaze.combined.gaze_2d.x/2), int(gaze.combined.gaze_2d.y/2))

                # YOLO 輸入準備
                blob = cv2.dnn.blobFromImage(buffer, 0.00392, (416, 416), (0, 0, 0), True, crop=False)
                net.setInput(blob)
                outs = net.forward(output_layers)

                # 儲存辨識結果
                class_ids = []
                confidences = []
                boxes = []

                # 處理每個輸出層
                for out in outs:
                    for detection in out:
                        scores = detection[5:]
                        class_id = np.argmax(scores)
                        confidence = scores[class_id]
                        
                        # 只保留高置信度的檢測
                        if confidence > 0.5:
                            center_x = int(detection[0] * width)
                            center_y = int(detection[1] * height)
                            w = int(detection[2] * width)
                            h = int(detection[3] * height)

                            # 邊界框座標
                            x = int(center_x - w / 2)
                            y = int(center_y - h / 2)

                            boxes.append([x, y, w, h])
                            confidences.append(float(confidence))
                            class_ids.append(class_id)

                # 非極大值抑制來過濾多餘的邊界框
                indexes = cv2.dnn.NMSBoxes(boxes, confidences, 0.5, 0.4)

                # 尋找指定座標內的物品
                target_item = None
                for i in range(len(boxes)):
                    if i in indexes:
                        x, y, w, h = boxes[i]
                        if x < center[0] < x + w and y < center[1] < y + h:  # 檢查座標是否在框內
                            target_item = classes[class_ids[i]]
                now_data["nowObject"] = target_item
                #print(target_item)
            except Exception as ex:
                print(ex)

            if now_data["nowObject"] != now_data["preObject"]:
                now_data["preObjectStart"] = -1
            elif now_data["preObjectStart"] == -1:
                now_data["preObjectStart"] = now_data["now"]

            if now_data["nowEyetrack"] != now_data["preEyetrack"]:
                now_data["preEyetrackStart"] = -1
            elif now_data["preEyetrackStart"] == -1:
                now_data["preEyetrackStart"] = now_data["now"]

            if now_data["preObjectStart"] != -1 and now_data["now"] - now_data["preObjectStart"] >= 30:
                if now_data["studyStartTime"] == -1 and (now_data["nowObject"] == "laptop" or now_data["nowObject"] == "book"):
                    now_data["studyStartTime"] = now_data["now"] - 30
                    now_data["studyTime"] = now_data["studyStartTime"]
                    now_data["studyDurationList"] = []
                    now_data["maxStudyDuration"] = 0
                    now_data["distractingStartTime"] = -1
                    now_data["distractionsInterval"] = 0
                    now_data["distractionsTimePerTenMin"] = []
                    now_data["overTenSec"] = False
            elif not(now_data["overTenSec"]) and now_data["studyStartTime"] == -1 and now_data["preObjectStart"] != -1 and now_data["now"] - now_data["preObjectStart"] >= 10:
                now_data["overTenSec"] = True

            if not(now_data["overTwentySec"]) and now_data["preEyetrackStart"] != -1 and now_data["now"] - now_data["preEyetrackStart"] >= 20:
                now_data["overTwentySec"] = True
            elif now_data["overTwentySec"] and now_data["preEyetrackStart"] == -1:
                now_data["overTwentySec"] = False
            
            if now_data["studyStartTime"] != -1:
                if now_data["distractingStartTime"] == -1 and (now_data["nowObject"] != "laptop" and now_data["nowObject"] != "book") and not(now_data["nowObject"] == None and now_data["preObject"] != None):
                    now_data["distractingStartTime"] = now_data["now"]
                    now_data["studyDurationList"].append(now_data["now"] - now_data["studyTime"])
                    now_data["maxStudyDuration"] = max(now_data["studyDurationList"])
                    now_data["studyTime"] = -1
                
                if now_data["distractingStartTime"] != -1:
                    if now_data["now"] - now_data["distractingStartTime"] >= 300:
                        now_data["studyStartTime"] = -1
                    elif now_data["nowObject"] == "laptop" or now_data["nowObject"] == "book":
                        now_data["distractionsInterval"] += now_data["now"] - now_data["distractingStartTime"]
                        now_data["distractingStartTime"] = -1
                        now_data["studyTime"] = now_data["now"]

                while now_data["studyStartTime"] != -1 and (now_data["now"] - now_data["studyStartTime"]) // 600 > len(now_data["distractionsTimePerTenMin"]):
                    now_data["distractionsTimePerTenMin"].append(now_data["distractionsInterval"] - [sum(now_data["distractionsTimePerTenMin"]), 0][len(now_data["distractionsTimePerTenMin"]) == 0])
            elif now_data["overTenSec"]:
                if now_data["preObjectStart"] == -1:
                    now_data["overTenSec"] = False

    except Exception as ex:
        print(ex)
    finally:
        th.cancel()
        th.join()
        cv2.destroyAllWindows()
        print('Stopped')

def run_flask():
    app.run(host="0.0.0.0", port=5000)

if __name__ == "__main__":
    threading.Thread(target=detect).start()
    threading.Thread(target=run_flask).start()
