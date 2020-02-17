constexpr size_t BUFFER_SIZE = 50;

constexpr char COMPLETE = '\n';

volatile char buffer[BUFFER_SIZE];
volatile size_t buffer_size;

int main(){
	enableISR();
	
	// if we have a complete buffer
	if (buffer_size != 0){
		// use the buffer
		//...
		
		// release the buffer
		buffer_size = 0;
	}
}

void isr(char c){
	static size_t counter = 0;
	
	//if the buffer is released
	if (buffer_size == 0){
		buffer[counter++] = c;
		if (c == COMPLETE){
			buffer_size = counter;
			counter = 0;
		}
	}else{
		//error byte lost!
	}
}



// ******************************************* alternative way, unless the HW implement it, you dont know if there is a lost message

constexpr size_t BUFFER_SIZE = 50;

volatile char buffer[BUFFER_SIZE];
volatile size_t buffer_size;

int main(){
	enableISR();
	
	// if we have a complete buffer
	if (!isISRenabled()){
		// use the buffer
		//...
		
		enableISR();
	}
}

void isr(){
	static size_t counter = 0;
	
	buffer[counter++] = c;
	if (c == COMPLETE){
		buffer_size = counter;
		disableISR();
	}
}

// as 1th option, but we hadle loss of bytes

constexpr size_t BUFFER_SIZE = 50;
constexpr char COMPLETE = '\n';

volatile char buffer[BUFFER_SIZE];
volatile size_t buffer_size;

int main(){
	enableISR();
	
	// if we have a complete buffer
	if (buffer_size != 0){
		// use the buffer
		//...
		
		// release the buffer
		buffer_size = 0;
	}
}

void isr(char c){
	static size_t counter = 0;
	static bool error_line = false;
	
	if (error_line){
		if (c == COMPLETE){
			error_line = false;
		}
		return;
	}
	
	//if the buffer is released
	if (buffer_size == 0){
		buffer[counter++] = c;
		if (c == COMPLETE){
			buffer_size = counter;
			counter = 0;
		}
	}else{
		//error byte lost! ignore the full next line
		error_line = true;
	}
}


// ******************************************* as 1th option, but we calculate start/end and also we calculate xor (NMEA like)

constexpr size_t BUFFER_SIZE = 50;

volatile char buffer[BUFFER_SIZE];
volatile size_t buffer_size;

int main(){
	enableISR();
	
	// if we have a complete buffer
	if (buffer_size){
		// use the buffer
		//...
		
		enableISR();
	}
}

void isr(char c){
	static size_t counter = 0;
	
	buffer[counter++] = c;
	if (c == COMPLETE){
		buffer_size = counter;
		disableISR();
	}
}

// ******************************************* as 1th option, but we hadle loss of bytes

constexpr size_t BUFFER_SIZE = 50;
constexpr char COMPLETE = '\n';

volatile char buffer[BUFFER_SIZE];
volatile size_t buffer_size;

int main(){
	enableISR();
	
	// if we have a complete buffer
	if (buffer_size != 0){
		// use the buffer
		//...
		
		// release the buffer
		enableISR();
	}
}

enum status{
	WAITING_START, WAITING_DATA, WAITING_CRC1, WAITING_CRC2
}

void isr(char c){
	static size_t counter = 0;
	static enum status current_status = WAITING_START;
	static uint8_t xor_calculated = 0;
	static uint8_t xor_got = 0;
	
	switch (current_status){
		case WAITING_START:
			if (c == '$'){
				current_status = WAITING_DATA;
				counter = 0;
				xor_calculated = 0;
			}
			break;
		case WAITING_DATA:
			if (c == '$'){
				counter = 0;
				xor_calculated = 0;
			}else if (c == '*'){
				current_status = WAITING_CRC1;
			}else{
				buffer[counter++] = c;
				xor_calculated ^= c;
			}
			break;
		case WAITING_CRC1:
			uint8_t half_byte = hex_to_byte(c);
			if (half_byte > 0x0f){
				//error invalid
				if (c == '$'){
					current_status = WAITING_DATA;
					counter = 0;
					xor_calculated = 0;
				}else{
					current_status = WAITING_START;
				}
			}else{
				xor_got = half_byte << 4;
				current_status = WAITING_CRC2;
			}
			break;
		case WAITING_CRC2:
			uint8_t half_byte = hex_to_byte(c);
			if (half_byte > 0x0f){
				//error invalid
				if (c == '$'){
					current_status = WAITING_DATA;
					counter = 0;
					xor_calculated = 0;
				}
			}else{
				xor_got |= half_byte;
				
				current_status = WAITING_START;
				
				if (xor_got == xor_calculated){
					buffer_size = counter;
					disableISR();
				}
			}
			break;
	}
}

// ******************************************* double buffer!

struct smart_array{
	char data[BUFFER_SIZE];
	size_t buffer_size;
};

volatile struct smart_array buffers[2] = {{0,0}, {0,0}};

volatile int buffer_ready = -1;

int main(){
	enableISR();
	
	if (buffer_ready >= 0) {
		// use the buffer
		//...
		
		// release the buffer
		buffer_ready = -1;
	}
}

void isr(char c){
	static bool current_buffer = 0;
	
	buffers[current_buffer].data[buffers[current_buffer].buffer_size++] = c;
	if (c == COMPLETE){
		if (buffer_ready == -1){
			buffer_ready = current_buffer;
			current_buffer = !current_buffer;
		}
	}else{
		// old buffer not available, override current one.
		// you should report this as an error/warning
		buffers[current_buffer].buffer_size = 0;
	}
}

// ******************************************* round buffer, but does NOT override old data
volatile struct smart_array{
	char data[BUFFER_SIZE];
	size_t buffer_start;
	size_t available;
} buffer;

int main(){
	enableISR();
	
	if (buffer.available) {
		char data = buffer.data[buffer.buffer_start];
		buffer.available--;

		//use data
	}
}

void isr(char c){
	if (buffer.available >= BUFFER_SIZE){
		// error buffer full
		return;
	}
	size_t index = buffer.buffer_start + buffer.available;
	buffer.data[buffer.buffer_start + buffer.available] = c;
	
}
