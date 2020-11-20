// shared mem to send 0 and 1 via not caching and caching the data
uint64_t buffer [24]; // 64*3 Byte (to ensure entire cache line length 64 Byte is occupied)
uint64_t* addr = (buffer+10);

void nop();
char* blubb();