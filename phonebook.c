/* phonebook.c
   Simple console phone book in C (C11)
   Compile: gcc phonebook.c -o phonebook
   Run: ./phonebook
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_CONTACTS 1000
#define MAX_STR 100
#define FILENAME "contacts.txt"

typedef struct {
    char firstName[50];
    char lastName[50];
    char phone[20];
    char email[50];
} Contact;

Contact contacts[MAX_CONTACTS];
int contact_count = 0;

/* Utility: trim newline */
void trim_newline(char *s) {
    size_t len = strlen(s);
    if (len > 0 && s[len-1] == '\n') s[len-1] = '\0';
}

/* Case-insensitive substring test */
int str_contains_ci(const char *hay, const char *needle) {
    if (!hay || !needle) return 0;
    char hay_l[MAX_STR], needle_l[MAX_STR];
    strncpy(hay_l, hay, MAX_STR-1); hay_l[MAX_STR-1]=0;
    strncpy(needle_l, needle, MAX_STR-1); needle_l[MAX_STR-1]=0;
    for (int i=0; hay_l[i]; ++i) hay_l[i] = tolower((unsigned char)hay_l[i]);
    for (int i=0; needle_l[i]; ++i) needle_l[i] = tolower((unsigned char)needle_l[i]);
    return strstr(hay_l, needle_l) != NULL;
}

/* Check duplicate by phone or exact first+last */
int is_duplicate(const Contact *c) {
    for (int i=0;i<contact_count;i++) {
        if (strcmp(contacts[i].phone, c->phone) == 0) return 1;
        if (strcmp(contacts[i].firstName, c->firstName) == 0 &&
            strcmp(contacts[i].lastName, c->lastName) == 0) return 1;
    }
    return 0;
}

void add_contact() {
    if (contact_count >= MAX_CONTACTS) { printf("Phonebook full.\n"); return; }
    Contact c;
    printf("Enter first name: ");
    if (!fgets(c.firstName, sizeof c.firstName, stdin)) return;
    trim_newline(c.firstName);

    printf("Enter last name: ");
    if (!fgets(c.lastName, sizeof c.lastName, stdin)) return;
    trim_newline(c.lastName);

    printf("Enter phone: ");
    if (!fgets(c.phone, sizeof c.phone, stdin)) return;
    trim_newline(c.phone);

    printf("Enter email: ");
    if (!fgets(c.email, sizeof c.email, stdin)) return;
    trim_newline(c.email);

    if (strlen(c.firstName)==0 || strlen(c.phone)==0) {
        printf("First name and phone are required.\n");
        return;
    }

    if (is_duplicate(&c)) {
        printf("Duplicate contact detected—abort.\n");
        return;
    }

    contacts[contact_count++] = c;
    printf("Contact added successfully!\n");
}

int find_index_by_name_or_phone(const char *key) {
    for (int i=0;i<contact_count;i++) {
        if (strcmp(contacts[i].phone, key) == 0) return i;
        if (strcmp(contacts[i].firstName, key) == 0) return i;
    }
    return -1;
}

void remove_contact() {
    char key[50];
    printf("Enter first name or phone of contact to remove: ");
    if (!fgets(key, sizeof key, stdin)) return;
    trim_newline(key);
    int idx = find_index_by_name_or_phone(key);
    if (idx == -1) {
        printf("Contact not found.\n");
        return;
    }
    printf("Found contact: %s %s, %s, %s\n",
           contacts[idx].firstName, contacts[idx].lastName,
           contacts[idx].phone, contacts[idx].email);
    printf("Are you sure you want to delete this contact? (y/N): ");
    char confirm[4];
    if (!fgets(confirm, sizeof confirm, stdin)) return;
    if (confirm[0]=='y' || confirm[0]=='Y') {
        /* shift left */
        for (int i=idx;i<contact_count-1;i++) contacts[i]=contacts[i+1];
        contact_count--;
        printf("Contact deleted.\n");
    } else {
        printf("Deletion canceled.\n");
    }
}

void search_contact() {
    char key[50];
    printf("Enter search term (first name, last name, or part of phone): ");
    if (!fgets(key, sizeof key, stdin)) return;
    trim_newline(key);
    int found = 0;
    for (int i=0;i<contact_count;i++) {
        if (str_contains_ci(contacts[i].firstName, key) ||
            str_contains_ci(contacts[i].lastName, key) ||
            str_contains_ci(contacts[i].phone, key)) {
            printf("[%d] %s %s | %s | %s\n", i+1,
                   contacts[i].firstName, contacts[i].lastName,
                   contacts[i].phone, contacts[i].email);
            found++;
        }
    }
    if (!found) printf("No matching contacts.\n");
}

int cmp_firstname(const void *a, const void *b) {
    const Contact *x=a, *y=b;
    return strcmp(x->firstName, y->firstName);
}
int cmp_lastname(const void *a, const void *b) {
    const Contact *x=a, *y=b;
    return strcmp(x->lastName, y->lastName);
}

void print_contacts() {
    if (contact_count == 0) { printf("No contacts.\n"); return; }
    printf("Sort by: 1) First name  2) Last name  (other -> no sort): ");
    char choice[4];
    if (!fgets(choice, sizeof choice, stdin)) return;
    if (choice[0] == '1') qsort(contacts, contact_count, sizeof(Contact), cmp_firstname);
    else if (choice[0] == '2') qsort(contacts, contact_count, sizeof(Contact), cmp_lastname);

    printf("Contacts (%d):\n", contact_count);
    for (int i=0;i<contact_count;i++) {
        printf("%d) %s %s | %s | %s\n", i+1,
               contacts[i].firstName, contacts[i].lastName,
               contacts[i].phone, contacts[i].email);
    }
}

void save_contacts_to_file() {
    FILE *f = fopen(FILENAME, "w");
    if (!f) { perror("Failed to open file for writing"); return; }
    for (int i=0;i<contact_count;i++) {
        /* escape commas by replacing them (simple approach) */
        fprintf(f, "%s,%s,%s,%s\n", contacts[i].firstName, contacts[i].lastName,
                contacts[i].phone, contacts[i].email);
    }
    fclose(f);
    printf("Contacts saved to %s\n", FILENAME);
}

void load_contacts_from_file() {
    FILE *f = fopen(FILENAME, "r");
    if (!f) { printf("No saved contacts found (%s).\n", FILENAME); return; }
    char line[512];
    contact_count = 0;
    while (fgets(line, sizeof line, f) && contact_count < MAX_CONTACTS) {
        trim_newline(line);
        /* parse CSV-like: first,last,phone,email */
        char *p = line;
        char *parts[4] = {0};
        for (int i=0;i<4;i++) {
            parts[i] = strsep(&p, ",");
            if (!parts[i]) parts[i] = "";
        }
        strncpy(contacts[contact_count].firstName, parts[0], sizeof contacts[0].firstName - 1);
        strncpy(contacts[contact_count].lastName, parts[1], sizeof contacts[0].lastName - 1);
        strncpy(contacts[contact_count].phone, parts[2], sizeof contacts[0].phone - 1);
        strncpy(contacts[contact_count].email, parts[3], sizeof contacts[0].email - 1);
        contact_count++;
    }
    fclose(f);
    printf("Loaded %d contacts from %s\n", contact_count, FILENAME);
}

void show_menu() {
    printf("\nPhone Book Menu\n");
    printf("1 — Add contact\n");
    printf("2 — Remove contact\n");
    printf("3 — Search contact\n");
    printf("4 — View all contacts\n");
    printf("5 — Save contacts\n");
    printf("6 — Load contacts\n");
    printf("0 — Exit\n");
    printf("Select option: ");
}

int main() {
    /* try auto-loading at startup */
    load_contacts_from_file();

    char input[8];
    while (1) {
        show_menu();
        if (!fgets(input, sizeof input, stdin)) break;
        switch (input[0]) {
            case '1': add_contact(); break;
            case '2': remove_contact(); break;
            case '3': search_contact(); break;
            case '4': print_contacts(); break;
            case '5': save_contacts_to_file(); break;
            case '6': load_contacts_from_file(); break;
            case '0': printf("Goodbye.\n"); return 0;
            default: printf("Unknown option.\n");
        }
    }
    return 0;
}
