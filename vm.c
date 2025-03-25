#define STACK_MAX 256

typedef enum {
	OBJ_INT,
	OBJ_PAIR
} ObjectType;

// The main Object struct has a type field that identifies what kind of value it is—either an int or a pair. 
// Then it has a union to hold the data for the int or pair. 
// A union is a struct where the fields overlap in memory. Since a given object can only be an int or a pair, there’s no reason to have memory in a single object for all three fields at the same time. A union does that. 
typedef struct sObject {
	ObjectType type;
	
	union {
		/* OBJ_INT */
		int value;
		
		/* OBJ_PAIR */
		struct {
			struct sObject* tail;
			struct sObject* head;
		};
	};

	unsigned char marked;
} Object;

// Define VM structure 
// stack is an array of Object pointers with size STACK_MAX
// stackSize represents the current size of the stack
typedef struct {
	Object* stack[STACK_MAX];
	int stackSize;
} VM;

// Function to create new VM
// Allocated memory in heap to be the size of the VM struct
// Initialises the stackSize to 0
// Return pointer to the VM
VM* newVM() {
	VM* vm = malloc(sizeof(VM));
	vm->stackSize = 0;
	return vm;
}

// Function to push new object pointer to stack
void push(VM* vm, Object* value) {
	assert(vm->stackSize < STACK_MAX, "Stack overflow!");
	vm->stack[vm->stackSize++] = value;
}

// Function to pop object from top of stack
// stackSize is decremented first
// Nothing is removed from memory
// The returned item is the item that has been 'popped' off, because indexes start from 0.
// Eg. if stackSize decrements from 3 to 2, it returns stack[2], which is the 3rd item.
Object* pop(VM* vm) {
	assert(vm->stackSize > 0, "Stack underflow!");
	return vm->stack[--vm->stackSize];
}

// Function to add new object
Object* newObject(VM* vm, ObjectType type) {
	Object* object = malloc(sizeof(Object));
	object->type = type;
	object->marked = 0;
	return object;
}

// Function to push new OBJ_INT
// Create a new Object of type INT
// Set the integer value
// Push it onto the VM stack
void pushInt(VM* vm, int intValue) {
	Object* object = newObject(vm, OBJ_INT);
	object->value = intValue;
	push(vm, object);
}

// Function to push new OBJ_PAIR
// Create a new Object of type PAIR
// Pop the stack to get the second value
// Pop again to get the first value
// Push the new pair object onto the stack
// Return the new pair
Object* pushPair(VM* vm) {
	Object* object = newObject(vm, OBJ_PAIR);
	object->tail = pop(vm);
	object->head = pop(vm);
	
	push(vm, object);
	return object;
}

// Mark function
void mark(Object* object) {
	// If already marked, return, otherwise objects could point to each other in a loop
	if (object-> marked) return;

	// Mark object
	object->marked = 1;

	// Recursively mark any objects that can be reached through this object
	if (object->type = OBJ_PAIR) {
		mark(object->head);
		mark(object->tail);
	}
}

// Mark all 
void markAll(VM* vm) {
	for (int i = 0; i < vm->stackSize; i++) {
		mark(vm->stack[i]);
	}
}
