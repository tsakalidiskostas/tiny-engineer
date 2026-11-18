# Desk corner

Optional GH-S37D build for a robot that runs off a power bank and sits on the corner of a desk. The body is rebuilt around the servo, the chair holds the power bank, and the lamp is replaced by desk props. Printables live under [`3d_models/parts/ghs37d/stl/`](3d_models/parts/ghs37d/stl/). There is no Fusion source yet.

The ESP32 board in this build has no RGB LED, so there is no colour status to show and the lamp is left out.

## Parts

| Role | Files |
| --- | --- |
| Body | `NewOrientationBody` (rebuilt body that houses the servo) |
| Chair | `chairForPowerbank` (chair with room for the power bank), `Overchair` (sits over the chair and holds the body in place; grips the horn instead of the servo, to match the new body) |
| Neck | `Neck` |
| Servo fit | `robotAdaptor` (adaptors from the GH-S37D's round horn hubs to the model's hexagonal sockets), `servoSleeve` (sleeve so a GH-S37D fits snugly in a pocket designed for the original servos) |
| Design aid | `motorPlaceholders` (a model of the GH-S37D, not printed; import it as a reference when you modify the parts) |
| Power | `powerInputSupport` (support for the power input) |
| Desk props | `headsetCollection` (headset, small headset holder and a mobile phone; replaces the lamp) |

## Notes

- Power comes from a power bank. The ESP32 board takes 5 V from the PCA9685 servo rail (green V+ terminal).
- If a servo horn sits loose on the gear, the joint twitches while the servo is unpowered. A thin strip of tape on the gear fixes it.
