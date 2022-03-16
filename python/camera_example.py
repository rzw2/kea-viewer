# %%
import chronoptics.tof as tof
import numpy as np
from matplotlib import cm as colormap
import cv2
import time

# %%


def displayConfig(config: tof.CameraConfig):
    for n in range(0, config.frameSize()):
        print("Frame {:d}".format(n))
        print("Modulation Frequency {:2.1f}".format(
            config.getModulationFrequency(n)))

        int_times = config.getIntegrationTime(n)

        print("Integration Times " + str(int_times))

        print("Duty Cycle: {:f}".format(config.getDutyCycle(n)))
        print("DAC: " + str(config.getDac()))

        phases = config.getPhaseShifts(n)
        print("Phase Shifts " + str(phases))

        binning = config.getBinning(n)
        print("Binning {:d}".format(binning))


def colourFrame(ToFframe: tof.Data, climit: list, cm="jet", badpixColor=[0, 0, 0]) -> np.ndarray:
    """
    Colour the depth frame into a Jet colour map 
    """
    frame = np.asarray(ToFframe)
    cmap = colormap.get_cmap(cm)

    blank_pix = np.flipud(np.isnan(frame) | (frame == 0))
    cm_frame = np.flipud(frame.copy())

    cm_frame[cm_frame < climit[0]] = climit[0]
    cm_frame[cm_frame > climit[1]] = climit[1]

    cm_frame = (cm_frame - climit[0]) / (climit[1] - climit[0])

    color_frame = cmap(cm_frame)

    # This converts to a BGR which OpenCV uses by default, instead of an RGB
    out_frame = np.flip(np.uint8(color_frame[:, :, 0:3] * 255), axis=2)
    out_frame[blank_pix] = badpixColor
    return out_frame


def quickRadialColor(ToFframe: tof.Data) -> np.ndarray:
    """
    If colouring the frame takes too long 
    """
    frame = np.asarray(ToFframe)
    return np.uint8(np.right_shift(frame, 8))

# %%


def main():
    # Camera processing settings
    serial = "202002a"
    fps = 15
    on_camera = False
    dmax = 30.0
    binning = 1

    user_config = tof.UserConfig()
    user_config.setFps(fps)
    user_config.setEnvironment(tof.ImagingEnvironment.SUNLIGHT)
    user_config.setMaxDistance(dmax)
    user_config.setStrategy(tof.Strategy.BALANCED)
    user_config.setIntegrationTime(tof.IntegrationTime.SHORT)

    cam = tof.KeaCamera(tof.ProcessingConfig(), serial)
    config = user_config.toCameraConfig(cam)
    for n in range(0, config.frameSize()):
        config.setBinning(n, binning)

    # Display the camera configuration
    displayConfig(config)

    # %%

    # Setup for displaying the FPS and how to display the depth image.
    jet = True
    counter, fps_m = 0, 0
    start_time = time.time()
    row_size = 20  # pixels
    left_margin = 24  # pixels
    text_color = (0, 0, 255)  # red
    font_size = 1
    font_thickness = 1
    fps_avg_frame_count = 10
    text_location = (left_margin, row_size)

    # %%

    cv2.namedWindow("Intensity")
    cv2.namedWindow("Depth")
    cv2.namedWindow("RGB")
    cv2.namedWindow("RGB Projected")

    proc = config.defaultProcessing()
    if on_camera:
        proc.setTemporalSigma(1.0)
    proc.setIntensityScale(10.0)

    cam.setCameraConfig(config)
    cam.setProcessConfig(proc)

    # Turn on camera processing
    if cam.onCameraProcessingCapable():
        cam.setOnCameraProcessing(on_camera)

    # Start the data stream from the camera
    tof.selectStreams(cam, [tof.FrameType.RADIAL,
                            tof.FrameType.BGR, tof.FrameType.INTENSITY, tof.FrameType.BGR_PROJECTED])

    cam.start()

    while cam.isStreaming():
        frames = cam.getFrames()
        counter += 1

        if counter % fps_avg_frame_count == 0:
            end_time = time.time()
            fps_m = fps_avg_frame_count / (end_time - start_time)
            start_time = time.time()

            # Hack way to measure fps
            #print("FPS {:f}".format(fps_m))
        fps_text = 'FPS = {:.1f}'.format(fps_m)

        for frame in frames:
            if frame.frameType() == tof.FrameType.INTENSITY:
                # Show the FPS
                img_intensity = np.flipud(np.asarray(frame))

                # Not working for these images
                # cv2.putText(img_intensity, fps_text, text_location, cv2.FONT_HERSHEY_PLAIN,
                #            font_size, text_color, font_thickness)

                cv2.imshow('Intensity', img_intensity)
            elif frame.frameType() == tof.FrameType.RADIAL:
                if jet:
                    img_color = colourFrame(frame, [0, 2**16])
                else:
                    img_color = quickRadialColor(frame)

                # cv2.putText(img_color, fps_text, text_location, cv2.FONT_HERSHEY_PLAIN,
                #            font_size, text_color, font_thickness)

                cv2.imshow('Depth', img_color)
            elif frame.frameType() == tof.FrameType.BGR:
                # Copy stops the crash, but no copy crashes the camera ..
                img_bgr = cv2.flip(np.asarray(frame).copy(), 0)

                # Going to resize to VGA, as displaying the entire image takes too much time.
                # img_bgr_vga = cv2.resize(
                #    img_bgr, (640, 480), interpolation=cv2.INTER_NEAREST)

                cv2.putText(img_bgr, fps_text, text_location, cv2.FONT_HERSHEY_PLAIN,
                            font_size, text_color, font_thickness)

                cv2.imshow('RGB', img_bgr)

            elif frame.frameType() == tof.FrameType.BGR_PROJECTED:
                img_bgr_proj = cv2.flip(np.asarray(frame), 0)
                # resize to QVGA to test the export of this pipeline .
                cv2.imshow('RGB Projected', img_bgr_proj)

            # Exscape key
            if cv2.waitKey(10) == 27:
                cam.stop()

    cv2.destroyAllWindows()


# %%
if __name__ == '__main__':
    main()
