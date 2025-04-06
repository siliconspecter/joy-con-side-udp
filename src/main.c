#include <stddef.h>
#include <windows.h>
#include <initguid.h>
#include <devguid.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <setupapi.h>
#include <hidclass.h>
#include <string.h>
#include "log.h"
#include "throw.h"
#include "malloc_or_throw.h"
#include "realloc_or_throw.h"
#include "../submodules/protocol/src/write_u32s.h"

const char *const prefix = "\\\\?\\hid#{00001124-0000-1000-8000-00805f9b34fb}_vid&0002057e_pid&200";
const char *const suffix = "&0000#{4d1e55b2-f16f-11cf-88cb-001111000030}";

int number_of_devices = 0;
char **device_paths = NULL;
int *device_path_lengths = NULL;
HANDLE *devices = NULL;

#define UPPER_LEFT 0
#define LOWER_LEFT 1
#define UPPER_RIGHT 2
#define LOWER_RIGHT 3

#define MAXIMUM_COMMAND_LENGTH 128

int main(const int argc, const char *const *const argv)
{
  (void)(argc);
  (void)(argv);

  const int prefix_length = strlen(prefix);
  const int suffix_length = strlen(suffix);

  if (argc != 3)
  {
    throw("Expected two arguments (recipient address and recipient port); actual quantity %d.", argc - 1);
  }

  const char *const recipient_address = argv[1];
  int recipient_port;
  char discard;

  if (sscanf(argv[2], "%d%c", &recipient_port, &discard) != 1)
  {
    throw("Failed to determine recipient port (\"%s\").", argv[2]);
  }

  if (recipient_port < 0 || recipient_port > UINT16_MAX)
  {
    throw("Recipient port %d out of range (0 - 65535).", recipient_port);
  }

  log("Starting Winsock...");

  WSADATA wsadata;

  const int winsock_result = WSAStartup(MAKEWORD(2, 2), &wsadata);
  if (winsock_result)
  {
    throw("Failed to start Winsock (error no. %d).", winsock_result);
  }

  log("Opening UDP socket...");

  const int socket_handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  if (socket_handle == -1)
  {
    throw("Failed to open UDP socket (error no. %d).", WSAGetLastError());
  }

  struct sockaddr_in address = {
      .sin_family = AF_INET,
      .sin_port = htons((uint16_t)recipient_port),
      .sin_addr = {
          .s_addr = inet_addr(recipient_address)},
  };

  log("Starting main loop...");

  uint8_t command[MAXIMUM_COMMAND_LENGTH + 5] = {0, 0, 0, 0, 3, 0, 0, 0};
  uint32_t command_length = 0;
  bool debounce_upper_left = false;
  bool debounce_lower_left = false;
  bool debounce_upper_right = false;
  bool debounce_lower_right = false;

  while (true)
  {
    HDEVINFO dev_info = SetupDiGetClassDevs(&GUID_DEVINTERFACE_HID, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

    if (dev_info == INVALID_HANDLE_VALUE)
    {
      throw("SetupDiGetClassDevs failed (error code %d).", GetLastError());
    }

    SP_DEVICE_INTERFACE_DATA device_interface_data = {.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA)};

    for (DWORD device_index = 0; SetupDiEnumDeviceInterfaces(dev_info, NULL, &GUID_DEVINTERFACE_HID, device_index, &device_interface_data); device_index++)
    {
      DWORD required_size = 0;

      if (SetupDiGetDeviceInterfaceDetail(dev_info, &device_interface_data, NULL, 0, &required_size, NULL))
      {
        throw("SetupDiGetDeviceInterfaceDetail was expected to fail so that its required size could be measured, but did not.");
      }

      const DWORD expected_error = GetLastError();

      if (expected_error != ERROR_INSUFFICIENT_BUFFER)
      {
        throw("SetupDiGetDeviceInterfaceDetail failed in an unexpected manner (error code %d).", expected_error);
      }

      if (required_size < sizeof(PSP_DEVICE_INTERFACE_DETAIL_DATA))
      {
        throw("Required size is less than expected.");
      }

      PSP_DEVICE_INTERFACE_DETAIL_DATA device_interface_detail_data = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc_or_throw(required_size);
      device_interface_detail_data->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

      if (!SetupDiGetDeviceInterfaceDetail(dev_info, &device_interface_data, device_interface_detail_data, required_size, NULL, NULL))
      {
        throw("SetupDiGetDeviceInterfaceDetail failed (error code %d).", GetLastError());
      }

      const int device_path_length = strlen(device_interface_detail_data->DevicePath);

      if (device_path_length > prefix_length + suffix_length)
      {
        if (strncmp(device_interface_detail_data->DevicePath, prefix, prefix_length) == 0)
        {
          switch (device_interface_detail_data->DevicePath[prefix_length])
          {
          case '6':
          case '7':
            if (strncmp(device_interface_detail_data->DevicePath + device_path_length - suffix_length, suffix, suffix_length) == 0)
            {
              bool is_new = true;

              for (int device_index = 0; device_index < number_of_devices; device_index++)
              {
                if (device_path_lengths[device_index] == device_path_length)
                {
                  if (strcmp(device_paths[device_index] + prefix_length + 1, device_interface_detail_data->DevicePath + prefix_length + 1) == 0)
                  {
                    is_new = false;
                    break;
                  }
                }
              }

              if (is_new)
              {
                HANDLE device = CreateFile(
                    device_interface_detail_data->DevicePath,
                    GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL);

                if (device == INVALID_HANDLE_VALUE)
                {
                  throw("CreateFile failed (error code %d).", GetLastError());
                }

                if (number_of_devices == 0)
                {
                  device_paths = malloc_or_throw(sizeof(const char *) * (number_of_devices + 1));
                  device_path_lengths = malloc_or_throw(sizeof(int) * (number_of_devices + 1));
                  devices = malloc_or_throw(sizeof(HANDLE) * (number_of_devices + 1));
                }
                else
                {
                  device_paths = realloc_or_throw(device_paths, sizeof(const char *) * (number_of_devices + 1));
                  device_path_lengths = realloc_or_throw(device_path_lengths, sizeof(int) * (number_of_devices + 1));
                  devices = realloc_or_throw(devices, sizeof(HANDLE) * (number_of_devices + 1));
                }

                char *const device_path_copy = malloc_or_throw(sizeof(char) * (device_path_length + 1));
                device_paths[number_of_devices] = device_path_copy;
                strcpy(device_path_copy, device_interface_detail_data->DevicePath);

                device_path_lengths[number_of_devices] = device_path_length;

                devices[number_of_devices] = device;

                number_of_devices++;

                log("Connected to device.");
              }
            }
            break;
          }
        }
      }

      free(device_interface_detail_data);
    }

    const DWORD error = GetLastError();

    if (error != ERROR_NO_MORE_ITEMS)
    {
      throw("Failed to enumerate HID devices (error code %d).", error);
    }

    if (!SetupDiDestroyDeviceInfoList(dev_info))
    {
      throw("SetupDiDestroyDeviceInfoList failed (error code %d).", GetLastError());
    }

    bool upper_left = false;
    bool lower_left = false;
    bool upper_right = false;
    bool lower_right = false;

    for (int device_index = 0; device_index < number_of_devices;)
    {
      uint8_t buffer[363];
      DWORD number_of_bytes_read;

      if (!ReadFile(devices[device_index], buffer, 363, &number_of_bytes_read, NULL))
      {
        const DWORD error = GetLastError();

        if (error == ERROR_DEVICE_NOT_CONNECTED)
        {
          if (!CloseHandle(devices[device_index]))
          {
            throw("CloseHandle failed (error code %d).", error);
          }

          free(device_paths[device_index]);

          number_of_devices--;

          for (int subsequent_device_index = device_index; subsequent_device_index < number_of_devices; subsequent_device_index++)
          {
            device_paths[subsequent_device_index] = device_paths[subsequent_device_index + 1];
            device_path_lengths[subsequent_device_index] = device_path_lengths[subsequent_device_index + 1];
            devices[subsequent_device_index] = devices[subsequent_device_index + 1];
          }

          if (number_of_devices == 0)
          {
            free(device_paths);
            free(device_path_lengths);
            free(devices);
          }
          else
          {
            device_paths = realloc_or_throw(device_paths, sizeof(const char *) * number_of_devices);
            device_path_lengths = realloc_or_throw(device_path_lengths, sizeof(int) * number_of_devices);
            devices = realloc_or_throw(devices, sizeof(HANDLE) * number_of_devices);
          }

          log("Disconnected from device.");

          continue;
        }
        else
        {
          throw("ReadFile failed (error code %d).", GetLastError());
        }
      }

      if (number_of_bytes_read != 362)
      {
        throw("Expected to read 362 bytes, actual %d.", number_of_bytes_read);
      }

      switch (device_paths[device_index][prefix_length])
      {
      case '6':
        upper_left = upper_left || buffer[5] & 32;
        lower_left = lower_left || buffer[5] & 16;
        break;

      case '7':
        upper_right = upper_right || buffer[3] & 16;
        lower_right = lower_right || buffer[3] & 32;
        break;
      }

      device_index++;
    }

    debounce_upper_left = debounce_upper_left && upper_left;
    debounce_lower_left = debounce_lower_left && lower_left;
    debounce_upper_right = debounce_upper_right && upper_right;
    debounce_lower_right = debounce_lower_right && lower_right;

    upper_left = upper_left && !debounce_upper_left;
    lower_left = lower_left && !debounce_lower_left;
    upper_right = upper_right && !debounce_upper_right;
    lower_right = lower_right && !debounce_lower_right;

    debounce_upper_left = debounce_upper_left || upper_left;
    debounce_lower_left = debounce_lower_left || lower_left;
    debounce_upper_right = debounce_upper_right || upper_right;
    debounce_lower_right = debounce_lower_right || lower_right;

    if (upper_left)
    {
      if (command_length < MAXIMUM_COMMAND_LENGTH)
      {
        command[command_length + 8] = UPPER_LEFT;
      }

      command_length++;
    }

    if (lower_left)
    {
      if (command_length < MAXIMUM_COMMAND_LENGTH)
      {
        command[command_length + 8] = LOWER_LEFT;
      }

      command_length++;
    }

    if (upper_right)
    {
      if (command_length < MAXIMUM_COMMAND_LENGTH)
      {
        command[command_length + 8] = UPPER_RIGHT;
      }

      command_length++;
    }

    if (lower_right)
    {
      if (command_length < MAXIMUM_COMMAND_LENGTH)
      {
        command[command_length + 8] = LOWER_RIGHT;
      }

      command_length++;
    }

    if (command_length > 0 && !debounce_upper_left && !debounce_lower_left && !debounce_upper_right && !debounce_lower_right)
    {
      if (command_length > MAXIMUM_COMMAND_LENGTH)
      {
        log("Command is too long.");
      }
      else
      {
        log("Sending %d-byte command...", command_length);

        command_length += 4;

        write_u32s(&command_length, 1, 0, command, MAXIMUM_COMMAND_LENGTH + 8);

        const size_t sent_bytes = sendto(socket_handle, (const char *const)command, command_length + 4, 0, (const struct sockaddr *const)&address, sizeof(address));

        if (sent_bytes == command_length + 4)
        {
          log("Sent %llu bytes.", sent_bytes);
        }
        else
        {
          log("Failed to send a message (error no. %d).", WSAGetLastError());
        }
      }

      command_length = 0;
    }

    Sleep(20);
  }
}
