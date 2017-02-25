using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace NZBDrive.Model
{
    public enum ThrottlingMode : int
    {
        [Description("Constant bitrate")]
        Constant = 0,
        [Description("Adaptive bitrate")]
        Adaptive = 1
    };
}
