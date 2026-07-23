# Experiment run log

Each row records one controlled experiment. The relay seed fixes the loss and
delay pattern for that run; only one protocol or delay change should be tested
at a time.

| Stage | Profile | Seed | Delay | Protection | Miss rate | Overhead | Result | Observation |
|---|---|---:|---:|---|---:|---:|---|---|
| Baseline | A | 1 | 120 ms | None | 2.27% | 1.02x | Invalid | One send per frame; misses closely track loss. |
| Baseline | B | 1 | 120 ms | None | 5.40% | 1.02x | Invalid | One send per frame; misses closely track loss. |
| Baseline | A | 1 | 60 ms | None | 2.27% | 1.02x | Invalid | Loss remains unrecovered. |
| Baseline | B | 1 | 60 ms | None | 38.73% | 1.02x | Invalid | The 20-80 ms path delay frequently exceeds the deadline. |
| FEC v1 | A | 1 | 120 ms | 4 data + 1 XOR parity | 0.27% | 1.34x | Valid | A single missing packet per four-frame block is recovered. |
| FEC v1 | B | 1 | 120 ms | 4 data + 1 XOR parity | 2.33% | 1.34x | Invalid | Multiple losses or a lost parity packet remain unrecoverable. |
| FEC v2 | A | 1 | 120 ms | 2 data + 1 XOR parity | 0.20% | 1.61x | Valid | Smaller blocks reduce both recovery delay and unrecoverable-loss probability. |
| FEC v2 | B | 1 | 120 ms | 2 data + 1 XOR parity | 0.80% | 1.61x | Valid | Current configuration; validate additional seeds before lowering delay. |
| FEC v2 | B | 2 | 120 ms | 2 data + 1 XOR parity | 0.53% | 1.61x | Valid | Multi-seed validation: valid. |
| FEC v2 | B | 3 | 120 ms | 2 data + 1 XOR parity | 0.47% | 1.61x | Valid | Multi-seed validation: valid. |
| FEC v2 | B | 4 | 120 ms | 2 data + 1 XOR parity | 0.20% | 1.61x | Valid | Multi-seed validation: valid. |
