#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

typedef uint32_t u32;
typedef int32_t i32;

struct process {
  u32 pid;
  u32 arrival_time;
  u32 burst_time;

  TAILQ_ENTRY(process) pointers;

  /* Additional fields here */
  u32 remaining_time;
  u32 waiting_time;
  u32 start_time;
  bool started;

  /* End of "Additional fields here" */
};

TAILQ_HEAD(process_list, process);

u32 next_int(const char **data, const char *data_end) {
  u32 current = 0;
  bool started = false;
  while (*data != data_end) {
    char c = **data;

    if (c < 0x30 || c > 0x39) {
      if (started) {
	return current;
      }
    }
    else {
      if (!started) {
	current = (c - 0x30);
	started = true;
      }
      else {
	current *= 10;
	current += (c - 0x30);
      }
    }

    ++(*data);
  }

  printf("Reached end of file while looking for another integer\n");
  exit(EINVAL);
}

u32 next_int_from_c_str(const char *data) {
  char c;
  u32 i = 0;
  u32 current = 0;
  bool started = false;
  while ((c = data[i++])) {
    if (c < 0x30 || c > 0x39) {
      exit(EINVAL);
    }
    if (!started) {
      current = (c - 0x30);
      started = true;
    }
    else {
      current *= 10;
      current += (c - 0x30);
    }
  }
  return current;
}

void init_processes(const char *path,
                    struct process **process_data,
                    u32 *process_size)
{
  int fd = open(path, O_RDONLY);
  if (fd == -1) {
    int err = errno;
    perror("open");
    exit(err);
  }

  struct stat st;
  if (fstat(fd, &st) == -1) {
    int err = errno;
    perror("stat");
    exit(err);
  }

  u32 size = st.st_size;
  const char *data_start = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (data_start == MAP_FAILED) {
    int err = errno;
    perror("mmap");
    exit(err);
  }

  const char *data_end = data_start + size;
  const char *data = data_start;
  

  *process_size = next_int(&data, data_end);

  *process_data = calloc(sizeof(struct process), *process_size);
  if (*process_data == NULL) {
    int err = errno;
    perror("calloc");
    exit(err);
  }

  for (u32 i = 0; i < *process_size; ++i) {
    (*process_data)[i].pid = next_int(&data, data_end);
    (*process_data)[i].arrival_time = next_int(&data, data_end);
    (*process_data)[i].burst_time = next_int(&data, data_end);
  }
  
  munmap((void *)data, size);
  close(fd);
}

int main(int argc, char *argv[])
{
  if (argc != 3) {
    return EINVAL;
  }
  struct process *data;
  u32 size;
  init_processes(argv[1], &data, &size);

  u32 quantum_length = next_int_from_c_str(argv[2]);

  struct process_list list;
  TAILQ_INIT(&list);

  u32 total_waiting_time = 0;
  u32 total_response_time = 0;

  /* Your code here */
  u32 t = 0;
  int done = 0;
  struct process *p;


  u32 first_arrival = 10000000;
  struct process *first_proc;

  for (int i = 0; i < size; i++) {
    
    // initialize processes
    p = &data[i];
    p->started = false;
    p->remaining_time = p->burst_time;
    
    // find first arrival
    if (data[i].arrival_time < first_arrival) {
      first_arrival = data[i].arrival_time;
      first_proc = &data[i];
    }
  }
  TAILQ_INSERT_HEAD(&list, first_proc, pointers);
  struct process *curr = first_proc;
  u32 time_slot = 0;

  while (done < size) {

    // check for newly-arrived processes
    for (int i = 0; i < size; i++) {
      if (data[i].arrival_time == t && &data[i] != first_proc) {
        TAILQ_INSERT_TAIL(&list, &data[i], pointers);
      }
    }

    // if process is just starting
    if (!curr->started) {
      curr->started = true;
      curr->start_time = t;
      total_response_time += t - curr->arrival_time;
    }

    time_slot++;
    t++;
    
    // remove process once finished
    curr->remaining_time--;
    if (curr->remaining_time == 0) {
      done++;
      printf("process %d arrived at ", curr->pid);
      printf("%d and finished at ", curr->arrival_time);
      printf("%d\n", t);
      total_waiting_time += t - curr->arrival_time - curr->burst_time;
      struct process *tmp = curr;
      curr = TAILQ_NEXT(tmp, pointers);
      TAILQ_REMOVE(&list, tmp, pointers);
      time_slot = 0;
    }

    // time slot is up, so move to next process
    else if (time_slot == quantum_length) {
      time_slot = 0;
      struct process *tmp = curr;
      curr = TAILQ_NEXT(tmp, pointers);
      if (curr == NULL) {
        curr = TAILQ_FIRST(&list);
      }
      TAILQ_REMOVE(&list, tmp, pointers);
      TAILQ_INSERT_TAIL(&list, tmp, pointers);
    }
    // TAILQ_FOREACH(p, &list, pointers) {
    //   if (p->started || p->arrival_time <= t) {
    //     if (!p->started) {
    //       p->started = true;
    //       p->start_time = t;
    //       total_response_time += t - p->arrival_time;
    //     }
    //     int passed = (((p->remaining_time) < (quantum_length)) ? (p->remaining_time) : (quantum_length));
    //     t += passed;
    //     p->remaining_time -= passed;
    //     printf("%d ran for ", p->pid);
    //     printf("%d seconds\n", passed);
    //     if (p->remaining_time == 0) {
    //       done++;
    //       total_waiting_time += t - p->arrival_time - p->burst_time;
    //       TAILQ_REMOVE(&list, p, pointers);
    //     }
    //   }
    // }
  }
  /* End of "Your code here" */

  printf("Average waiting time: %.2f\n", (float) total_waiting_time / (float) size);
  printf("Average response time: %.2f\n", (float) total_response_time / (float) size);

  free(data);
  return 0;
}
