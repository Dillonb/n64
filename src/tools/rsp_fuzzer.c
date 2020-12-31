#include <stdio.h>
#include <ftd2xx.h>
#include <log.h>
#include <stdbool.h>
#include <string.h>
#include <rsp_types.h>
#include <rsp_vector_instructions.h>

static unsigned int num_devices;
static FT_DEVICE_LIST_INFO_NODE* device_info;
static FT_HANDLE handle;

unsigned int bytes_written;
unsigned int bytes_read;

typedef union flag_result {
    struct {
        half vcc;
        half vco;
        half vce;
        half padding;
    };
    dword packed;
} flag_result_t;
static_assert(sizeof(flag_result_t) == sizeof(dword), "flag_result_t should be 64 bits");

void assert_ftcommand(FT_STATUS result, const char* message) {
    if (result != FT_OK) {
        logdie("%s", message);
    }
}

bool is_everdrive(unsigned int device) {
    return (strcmp(device_info[device].Description, "FT245R USB FIFO") == 0 && device_info[device].ID == 0x04036001);
}

void run_command(char* command, size_t command_length, char* response, size_t response_size) {
    memset(response, 0, response_size);

    // Send command
    assert_ftcommand(FT_Write(handle, command, command_length, &bytes_written), "Unable to write command!");
    printf("Sent to ED64: ");
    for (int i = 0; i < command_length; i++) {
        printf("%c", command[i]);
    }
    printf("\n");
    // Read response
    assert_ftcommand(FT_Read(handle, response, response_size, &bytes_read), "Unable to read command response!");

    printf("Received back from ED64: %s\n", response);
}

void send_vreg(vu_reg_t* reg) {
    vu_reg_t swapped;
    memcpy(&swapped, reg, sizeof(vu_reg_t));

    for (int i = 0; i < 8; i++) {
        swapped.elements[i] = be16toh(swapped.elements[i]);
    }

    assert_ftcommand(FT_Write(handle, &swapped, sizeof(vu_reg_t), &bytes_written), "Unable to write vu reg!");
}

void recv_vreg(vu_reg_t* reg) {
    assert_ftcommand(FT_Read(handle, reg, sizeof(vu_reg_t), &bytes_read), "Unable to read vu reg!");
    for (int i = 0; i < 8; i++) {
        reg->elements[i] = be16toh(reg->elements[i]);
    }
}

void recv_flag_result(flag_result_t* result) {
    assert_ftcommand(FT_Read(handle, result, sizeof(flag_result_t), &bytes_read), "Unable to read flag_result!");
    // TODO probably gonna need to bswap
}

void init_everdrive(unsigned int device) {
    // initialize
    assert_ftcommand(FT_Open(device, &handle), "Unable to open device!");
    assert_ftcommand(FT_ResetDevice(handle), "Unable to reset device!");
    assert_ftcommand(FT_SetTimeouts(handle, 5000, 5000), "Unable to set timeouts!");
    assert_ftcommand(FT_Purge(handle, FT_PURGE_RX | FT_PURGE_TX), "Unable to purge USB buffer contents!");
}

void print_vureg_ln(vu_reg_t* reg) {
    for (int i = 0; i < 8; i++) {
        printf("%04X ", reg->elements[i]);
    }
    printf("\n");
}

void print_vureg_comparing_ln(vu_reg_t* reg, vu_reg_t* compare) {
    for (int i = 0; i < 8; i++) {
        if (compare->elements[i] != reg->elements[i]) {
            printf(COLOR_RED);
        }
        printf("%04X ", reg->elements[i]);
        if (compare->elements[i] != reg->elements[i]) {
            printf(COLOR_END);
        }
    }
    printf("\n");
}

void check_emu(vu_reg_t vs, vu_reg_t vt, int e, vu_reg_t* res, vu_reg_t* acc_h, vu_reg_t* acc_m, vu_reg_t* acc_l) {
    rsp_t rsp;
    memset(&rsp, 0, sizeof(rsp_t));

    mips_instruction_t instruction;
    instruction.raw = 0;

    instruction.cp2_vec.vs = 1;
    instruction.cp2_vec.vt = 2;
    instruction.cp2_vec.vd = 3;
    instruction.cp2_vec.e = e;


    for (int i = 0; i < 8; i++) {
        rsp.vu_regs[1].elements[7 - i] = vs.elements[i];
        rsp.vu_regs[2].elements[7 - i] = vt.elements[i];
    }

    rsp_vec_vadd(&rsp, instruction);

    for (int i = 0; i < 8; i++) {
        res->elements[i] = rsp.vu_regs[3].elements[7 - i];
        acc_h->elements[i] = rsp.acc.h.elements[7 - i];
        acc_m->elements[i] = rsp.acc.m.elements[7 - i];
        acc_l->elements[i] = rsp.acc.l.elements[7 - i];
    }
}

int main(int argc, char** argv) {
    if (FT_CreateDeviceInfoList(&num_devices) != FT_OK) {
        logdie("Unable to enumerate num_devices. Try again?");
    }

    if (num_devices == 0) {
        logdie("No devices found. Is your EverDrive plugged in?");
    }

    logalways("Found %d device%s.", num_devices, num_devices == 1 ? "" : "s");

    device_info = malloc(sizeof(FT_DEVICE_LIST_INFO_NODE) * num_devices);
    FT_GetDeviceInfoList(device_info, &num_devices);

    int everdrive_index = -1;
    for (int i = 0; i < num_devices && everdrive_index < 0; i++) {
        if (is_everdrive(i)) {
            everdrive_index = i;
        }
    }

    if (everdrive_index < 0) {
        logdie("Did not find an everdrive!\n");
    }

    init_everdrive(everdrive_index);

    vu_reg_t arg1;
    vu_reg_t arg2;

    arg1.elements[0] = 0x0001;
    arg1.elements[1] = 0x0002;
    arg1.elements[2] = 0x0003;
    arg1.elements[3] = 0x0004;
    arg1.elements[4] = 0x0005;
    arg1.elements[5] = 0x0006;
    arg1.elements[6] = 0x0007;
    arg1.elements[7] = 0x0008;

    arg2.elements[0] = 0x0009;
    arg2.elements[1] = 0x000a;
    arg2.elements[2] = 0x000b;
    arg2.elements[3] = 0x000c;
    arg2.elements[4] = 0x000d;
    arg2.elements[5] = 0x000e;
    arg2.elements[6] = 0x000f;
    arg2.elements[7] = 0x0010;

    send_vreg(&arg1);
    send_vreg(&arg2);

    print_vureg_ln(&arg1);
    printf("VADD\n");
    print_vureg_ln(&arg2);
    printf("Result:\n");

    for (int element = 0; element < 16; element++) {
        vu_reg_t res;
        recv_vreg(&res);

        vu_reg_t acc_h;
        recv_vreg(&acc_h);

        vu_reg_t acc_m;
        recv_vreg(&acc_m);

        vu_reg_t acc_l;
        recv_vreg(&acc_l);

        flag_result_t flag_result;
        recv_flag_result(&flag_result);

        vu_reg_t emu_res;
        vu_reg_t emu_acc_h;
        vu_reg_t emu_acc_m;
        vu_reg_t emu_acc_l;
        check_emu(arg1, arg2, element, &emu_res, &emu_acc_h, &emu_acc_m, &emu_acc_l);


        printf("element %02d, n64 res: ", element);
        print_vureg_ln(&res);
        printf("element %02d, emu res: ", element);
        print_vureg_comparing_ln(&emu_res, &res);

        printf("element %02d, n64 acc_h: ", element);
        print_vureg_ln(&acc_h);
        printf("element %02d, emu acc_h: ", element);
        print_vureg_comparing_ln(&emu_acc_h, &acc_h);

        printf("element %02d, n64 acc_m: ", element);
        print_vureg_ln(&acc_m);
        printf("element %02d, emu acc_m: ", element);
        print_vureg_comparing_ln(&emu_acc_m, &acc_m);

        printf("element %02d, n64 acc_l: ", element);
        print_vureg_ln(&acc_l);
        printf("element %02d, emu acc_l: ", element);
        print_vureg_comparing_ln(&emu_acc_l, &acc_l);
        printf("\n");

        printf("element %02d, n64 vcc: 0x%04X\n", element, flag_result.vcc);
        printf("element %02d, n64 vco: 0x%04X\n", element, flag_result.vco);
        printf("element %02d, n64 vce: 0x%04X\n", element, flag_result.vce);
    }
}