using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace SystemXMLParser
{
    public interface ICollideable
    {
        bool Collides(ICollideable other);
    }
}
