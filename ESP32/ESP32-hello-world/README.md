# ESP32 Hello World

When JTAG doesn't work, often for infinite-loop failing app,
this simple "Hello World" app, programmed via serial port can help.

The file size for the enclosed `esptool.exe` 3,491,344 bytes and the SHA-256 value is:

`1B-EB-B7-CA-09-F0-4F-98-A7-93-DC-EE-F6-19-C4-78-78-7F-08-9E-DA-14-E4-74-76-B6-5A-B6-13-4D-25-9D`



Here's a sample SHA-256 file calculation in C#

```
using System.Security.Cryptography;
using System.Text;
using System.IO;

namespace TestdriveSHA256
{
    class Program
    {
        static void Main(string[] args)
        {
            using (SHA256 mySHA256 = SHA256.Create())
            {
                byte[] stringBytes = Encoding.ASCII.GetBytes("Hello World");
                stringBytes = File.ReadAllBytes(@"C:\workspace\wolfssl-examples\ESP32\ESP32-hello-world\esptool.exe");
                byte[] hashValue = mySHA256.ComputeHash(stringBytes);
                string resultValue = System.BitConverter.ToString(hashValue);
                System.Console.WriteLine(resultValue);
            }
        }
    }
}
```

<br />

## Support

Please contact wolfSSL at support@wolfssl.com with any questions, bug fixes,
or suggested feature additions.