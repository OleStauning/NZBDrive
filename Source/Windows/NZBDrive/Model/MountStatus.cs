using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace NZBDrive.Model
{
    public enum MountStatus : int
    {
        [Description("Mounting")]
        Mounting = 0,
        [Description("Downloaded")]
        Mounted = 1,
    }
}
