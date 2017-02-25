using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace NZBDrive.Model
{
    public enum NewsServerConnectionStatus : int
    {
        Disconnected, 
        Connecting, 
        Idle, 
        Working
    }
}
