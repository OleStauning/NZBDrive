using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace NZBDrive.Model
{
    public enum DataRateUnit : int
    {
        [Description("KB/s")]
        KiBs = 0,
        [Description("MB/s")]
        MiBs = 1,
        [Description("GB/s")]
        GiBs = 2,
        [Description("Unlimited")]
        Unlimited = 3,
    }
}
