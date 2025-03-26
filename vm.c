#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#define STACK_MAX 256
#define INITIAL_GC_THRESHHOLD 100

typedef enum {
	OBJ_INT,
	OBJ_PAIR
} ObjectType;

// The main Object struct has a type field that identifies what kind of value it is—either an int or a pair. 
// Then it has a union to hold the data for the int or pair. 
// A union is a struct where the fields overlap in memory. Since a given object can only be an int or a pair, there’s no reason to have memory in a single object for all three fields at the same time. A union does that. 
typedef struct sObject {
	struct sObject* next; // Pointer to next object in allocated list

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
	Object* firstObject; // Most recently allocated object
	Object* stack[STACK_MAX];
	int stackSize;

	int numObjects;
	int maxObjects; // Number of objects required to trigger a garbage collection
} VM;

// Function to create new VM
// Allocated memory in heap to be the size of the VM struct
// Initialises the stackSize to 0
// Return pointer to the VM
VM* newVM() {
	VM* vm = malloc(sizeof(VM));
	vm->stackSize = 0;
	vm->firstObject = NULL;
	vm->numObjects = 0;
	vm->maxObjects = INITIAL_GC_THRESHHOLD;
	return vm;
}

// Function to push new object pointer to stack
void push(VM* vm, Object* value) {
	if (vm->stackSize >= STACK_MAX) {
		fprintf(stderr, "Stack overflow!\n");
		exit(1);
	}
	vm->stack[vm->stackSize++] = value;
}

// Function to pop object from top of stack
// stackSize is decremented first
// Nothing is removed from memory
// The returned item is the item that has been 'popped' off, because indexes start from 0.
// Eg. if stackSize decrements from 3 to 2, it returns stack[2], which is the 3rd item.
Object* pop(VM* vm) {
	if (vm->stackSize == 0) {
		fprintf(stderr, "Stack underflow!\n");
		exit(1);
	}
	// Get the object that will be popped
	Object* object = vm->stack[--vm->stackSize];

	// Print the object being popped (for debugging)
	printf("[POP] Popped object: %p, Type: %d\n", (void*)object, object->type);

	return object;
}

void gc(VM* vm); // Function prototype

// Function to add new object
Object* newObject(VM* vm, ObjectType type) {
	// Run GC if reached max objects
	if (vm->numObjects == vm->maxObjects) gc(vm);

	Object* object = malloc(sizeof(Object));
	object->type = type;
	object->marked = 0;

	// Assign new object to head of object allocation list and assign previous head to the next prop of the new object
	object->next = vm->firstObject;
	vm->firstObject = object;

	vm->numObjects++;

	printf("[ALLOC] Object created: %p, Total Objects: %d\n", (void*)object, vm->numObjects);
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
	if (object->type == OBJ_PAIR) {
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


// Sweep
// Example: vm->firstObject is stored at address 0x400 and contains 0x100, which is the addrss of object 1. object is initialised to contain 0x400.
// Let's say object 1 is marked. Since object is 0x400, *object is 0x100. So (*object)->marked = true. That mean we unamrk it and set object to be &(*object)->next, which is the address of Object 1's next pointer, e.g 0x110. 0x100 contains the value 0x200 (the address of object 2).
// Now object = 0x110 (Obj 1's next address) and *object = 0x200 (Obj 2's address). Let's say Obj 2 is unmarked, so (*object)->marked = false. We create a new pointer unreached and set it to be 0x200, the address of Obj 2. Then we set *object (Obj 1's next value) to be unreached->next, whicih is 0x300, the address of Obj 3.
// So now, object is still 0x110, but 0x110 stores the address of Obj 3, 0x300. We have updated the list to skip out Obj 2. Finally, we free the memory at 0x200.
void sweep(VM* vm)
{
	Object** object = &vm->firstObject;
	while (*object) {
		if (!(*object)->marked) {
			// Object not marked. Save the address of the unmarked object in unreached
			Object* unreached = *object;

			// Adjust the current object pointer to contain the address of the next item in the list.
			*object = unreached->next;

			// Free the unmarked object from memory
			printf("[FREE] Object at: %p\n", (void*)unreached);
			free(unreached);
			vm->numObjects--;
		} else {
			// Object is marked. Unmark object.
			(*object)->marked = 0;

			// Adjust object pointer to point to the address of current object's next pointer.
			object = &(*object)->next;
		}
	}
}

void gc(VM* vm) {
	printf("\n[GC START] Allocated: %d, Threshold: %d\n", vm->numObjects, vm->maxObjects);

	int before = vm->numObjects;

	markAll(vm);
	sweep(vm);

	int after = vm->numObjects;
	vm->maxObjects = after * 2;

	printf("[GC END] Freed: %d, Remaining: %d, New Threshold: %d\n\n", before - after, after, vm->maxObjects);
}


// DEMO: Run the program 
int main() {
	VM* vm = newVM();

	// Push 80 int's
	for (int i = 0; i < 80; i++) {
		pushInt(vm, i);
	}
	
	// Pop some off the VM stack
	for (int i = 0; i < 10; i++) {
		pop(vm);
	}
	
	// Trigger garbage collection by exceeding 100 allocated objects
	for (int i = 0; i < 21; i++) {
		pushInt(vm, i);
	}

	printf("Done allocating objects!\n");

	return 0;
}