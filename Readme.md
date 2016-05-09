#Task System

This repository contains a comparison of multiple task queuing implmentations.

We have a typical task system based on a single queue feeding multiple threads, a queue-per-thread system and finally queue-per-thread with work stealing.

A minimal set of tests are used to time the various implementations using tasks that are side effect only.

