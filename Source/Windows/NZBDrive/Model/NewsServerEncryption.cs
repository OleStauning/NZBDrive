using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace NZBDrive.Model
{
    public enum NewsServerEncryption : int
    {
        [Description("None")]
        None = 0,
        [Description("SSL")]
        SSL = 1
    }
}
