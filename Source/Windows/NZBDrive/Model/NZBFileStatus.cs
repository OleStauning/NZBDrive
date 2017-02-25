using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace NZBDrive.Model
{
    public enum NZBFileStatus
    {
        [Description("OK")]
        OK = 0,
        [Description("Warning")]
        Warning = 1,
        [Description("Error")]
        Error = 2,
    }
}
