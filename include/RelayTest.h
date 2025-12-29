#ifndef RELAY_TEST_H
#define RELAY_TEST_H

// Direct relay test - toggles every 3 seconds, tests both Active HIGH and LOW
void testRelayDirect();

// Manual relay test - control via serial commands (h/l/r)
void testRelayManual();

#endif // RELAY_TEST_H
