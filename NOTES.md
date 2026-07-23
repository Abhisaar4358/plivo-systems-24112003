# Notes

The sender groups two consecutive 160-byte frames and sends both data packets plus one XOR parity packet.
The receiver forwards received data immediately, stores recent blocks in a bounded ring, and reconstructs a missing member when it has the parity packet and the other data packet.
All custom relay packets use a compact 12-byte header and a 160-byte body, while the harness-facing packets retain the required sequence-plus-payload format.
The current grading delay is 120 ms, selected because it is valid on both supplied profiles before latency tuning.
This configuration uses about 1.61x total relay bandwidth and no feedback traffic.
It cannot recover a block when both its data and parity protection are unavailable, nor can it correct corrupted packets.
Burst losses or a tighter deadline can therefore cause misses, and the final delay will be revised only after multi-seed validation.
