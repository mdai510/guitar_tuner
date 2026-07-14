from __future__ import annotations

import math
import sys

import matplotlib.pyplot as plt
import serial
from serial import SerialException


SERIAL_PORT = "COM6"
BAUD_RATE = 115200
SERIAL_TIMEOUT_SECONDS = 0.01

PLOT_MIN_FREQUENCY_HZ = 40.0
PLOT_MAX_FREQUENCY_HZ = 1000.0

USE_DECIBELS = True
MAGNITUDE_FLOOR = 1.0e-20


def convert_to_decibels(magnitudes: list[float]) -> list[float]:
    db_values = [
        10.0 * math.log10(max(value, MAGNITUDE_FLOOR))
        for value in magnitudes
    ]

    peak_db = max(db_values)

    return [value - peak_db for value in db_values]


def main() -> None:
    try:
        serial_port = serial.Serial(
            port=SERIAL_PORT,
            baudrate=BAUD_RATE,
            timeout=SERIAL_TIMEOUT_SECONDS,
        )
    except SerialException as exc:
        print(f"Could not open {SERIAL_PORT}: {exc}", file=sys.stderr)
        raise SystemExit(1) from exc

    print(f"Listening on {SERIAL_PORT} at {BAUD_RATE} baud")

    plt.ion()

    figure, axis = plt.subplots()
    line, = axis.plot([], [])

    axis.set_title("STM32 Live FFT Spectrum")
    axis.set_xlabel("Frequency (Hz)")
    axis.set_xlim(PLOT_MIN_FREQUENCY_HZ, PLOT_MAX_FREQUENCY_HZ)
    axis.grid(True)

    if USE_DECIBELS:
        axis.set_ylabel("Relative power (dB)")
        axis.set_ylim(-80.0, 5.0)
    else:
        axis.set_ylabel("Magnitude squared")

    receiving_frame = False
    frequencies: list[float] = []
    magnitudes: list[float] = []

    try:
        while plt.fignum_exists(figure.number):
            # Read only one available line per loop iteration.
            raw_line = serial_port.readline()

            if raw_line:
                line_text = raw_line.decode(
                    "utf-8",
                    errors="ignore",
                ).strip()

                # Uncomment this while debugging the serial format.
                print(repr(line_text))

                if line_text == "BEGIN":
                    frequencies.clear()
                    magnitudes.clear()
                    receiving_frame = True

                elif line_text == "END":
                    if receiving_frame and frequencies:
                        if USE_DECIBELS:
                            display_values = convert_to_decibels(
                                magnitudes
                            )
                        else:
                            display_values = magnitudes

                        line.set_data(frequencies, display_values)

                        if not USE_DECIBELS:
                            axis.relim()
                            axis.autoscale_view(
                                scalex=False,
                                scaley=True,
                            )

                        figure.canvas.draw_idle()

                    receiving_frame = False

                elif receiving_frame:
                    try:
                        frequency_text, magnitude_text = (
                            line_text.split(",", maxsplit=1)
                        )

                        frequency = float(frequency_text)
                        magnitude = float(magnitude_text)

                        if (
                            PLOT_MIN_FREQUENCY_HZ
                            <= frequency
                            <= PLOT_MAX_FREQUENCY_HZ
                        ):
                            frequencies.append(frequency)
                            magnitudes.append(magnitude)

                    except ValueError:
                        # Ignore malformed lines and other debug messages.
                        pass

            # Always allow Matplotlib to process window events,
            # even when no serial data has arrived.
            plt.pause(0.001)

    except KeyboardInterrupt:
        pass
    finally:
        serial_port.close()
        plt.ioff()
        plt.close(figure)


if __name__ == "__main__":
    main()