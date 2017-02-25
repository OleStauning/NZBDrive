using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace NZBDrive.Model
{
    public enum DataSizeUnit : int
    {
        [Description("KB")]
        KiB = 0,
        [Description("MB")]
        MiB = 1,
        [Description("GB")]
        GiB = 2,
    }

}
