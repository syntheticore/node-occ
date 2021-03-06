#include "Edge.h"

#include "Util.h"
#include "Mesh.h"

#include <assert.h>

bool Edge::isSeam(Base *face)
{
  if (this->shape().IsNull())
    return false;
  return BRep_Tool::IsClosed(this->edge(), TopoDS::Face(face->shape())) ? true: false;
}

bool Edge::isDegenerated()
{
  if (this->shape().IsNull())
    return true;
  return BRep_Tool::Degenerated(this->edge()) ? true: false;
}

bool Edge::isClosed()
{
  if (this->shape().IsNull())
    return false;
  TopoDS_Vertex aV1, aV2;
  TopExp::Vertices(this->edge(), aV1, aV2);
  if (!aV1.IsNull() && !aV2.IsNull() && aV1.IsSame(aV2))
    return true;
  return false;
}

int Edge::numVertices()
{
  TopTools_IndexedMapOfShape anIndices;
  TopExp::MapShapes(this->edge(), TopAbs_VERTEX, anIndices);
  return anIndices.Extent();
}

double Edge::length()
{
  if (edge().IsNull()) return 0.0;

  GProp_GProps prop;
  BRepGProp::LinearProperties(this->edge(), prop);
  return prop.Mass();
}



int Edge::createLine(Vertex *start, Vertex *end)
{

  try {
    gp_Pnt aP1 = start->point();
    gp_Pnt aP2 = end->point();

    GC_MakeLine line(aP1, aP2);

    TopoDS_Shape shape = BRepBuilderAPI_MakeEdge(line, start->vertex(), end->vertex());
    this->setShape(shape);

  }
  CATCH_AND_RETHROW("Failed to create line");

  return 1;
}

int Edge::createArc(Vertex *start, Vertex *end, const gp_Pnt& center)
{
  try {
    gp_Pnt aP1 = start->point();
    gp_Pnt aP2 = center;
    gp_Pnt aP3 = end->point();

    Standard_Real Radius = aP1.Distance(aP2);
    gce_MakeCirc MC(aP2,gce_MakePln(aP1, aP2, aP3).Value(), Radius);
    const gp_Circ& Circ = MC.Value();

    Standard_Real Alpha1 = ElCLib::Parameter(Circ, aP1);
    Standard_Real Alpha2 = ElCLib::Parameter(Circ, aP3);
    occHandle(Geom_Circle) C = new Geom_Circle(Circ);
    occHandle(Geom_TrimmedCurve) arc = new Geom_TrimmedCurve(C, Alpha1, Alpha2, false);

    this->setShape(BRepBuilderAPI_MakeEdge(arc, start->vertex(), end->vertex()));

  }
  CATCH_AND_RETHROW("Failed to create arc");

  return 1;
}

int Edge::createArc3P(Vertex *start, Vertex *end, const gp_Pnt& aPoint)
{
  try {
    gp_Pnt aP1 = start->point();
    gp_Pnt aP2 = aPoint;
    gp_Pnt aP3 = end->point();
    GC_MakeArcOfCircle arc(aP1, aP2, aP3);
    this->setShape(BRepBuilderAPI_MakeEdge(arc, start->vertex(), end->vertex()));
  }
  CATCH_AND_RETHROW("Failed to create arc");
  return 1;
}

int Edge::createCircle(const gp_Pnt& center, const gp_Dir& normal, double radius)
{
  try {
    gp_Pnt aP1= center;
    gp_Dir aD1= normal;

    if (radius <= Precision::Confusion()) {
      StdFail_NotDone::Raise("radius to small");
    }

    gce_MakeCirc circle(aP1, aD1, radius);
    this->setShape(BRepBuilderAPI_MakeEdge(circle));

  }
  CATCH_AND_RETHROW("Failed to create circle");
  return 1;
}


Vertex* getOrCreateVertex(Handle<Value> arg)
{
  if(arg->IsArray()) {

    Local<Value> objV = NanNew<v8::FunctionTemplate>(Vertex::_template)->GetFunction()->CallAsConstructor(1,&arg);
    if (!NanHasInstance(Vertex::_template,objV)) {
      return 0;
    }
    Vertex* vertex = node::ObjectWrap::Unwrap<Vertex>(objV->ToObject());
    return vertex;

  }     else if(arg->IsObject()) {

    Handle<Object> obj = arg->ToObject();
    if (!NanHasInstance(Vertex::_template,obj)) {
      return 0;
    }
    Vertex* vertex =node::ObjectWrap::Unwrap<Vertex>(obj);
    return vertex;
  } else {
    return 0;
  }
}

NAN_METHOD(Edge::createLine)
{
  NanScope();

  Local<v8::Value> arg1 = args[0];
  Local<v8::Value> arg2 = args[1];
  if (arg1.IsEmpty()|| arg2.IsEmpty()) {
    NanThrowError("expecting 2 arguments : <vertex|point> , <vertex|point> ");
    NanReturnUndefined();

  }

  Vertex* v1 =     getOrCreateVertex(args[0]);
  Vertex* v2 =     getOrCreateVertex(args[1]);
  if (!v1 || !v2) {
    NanThrowError("expecting 2 arguments : <vertex|point> , <vertex|point> ");
    NanReturnUndefined();
  }

  Edge* pThis = ObjectWrap::Unwrap<Edge>(args.This());


  pThis->createLine(v1,v2);
  NanReturnValue(args.This());
}

NAN_METHOD(Edge::createCircle)
{
  NanScope();

  Local<v8::Value> arg1 = args[0];
  Local<v8::Value> arg2 = args[1];
  Local<v8::Value> arg3 = args[2];

  if (arg1.IsEmpty()|| arg2.IsEmpty() || arg3.IsEmpty()) {
    NanThrowError("expecting three arguments : <center>,<normal>,<radius>");
    NanReturnUndefined();
  }

  gp_Pnt center;
  ReadPoint(arg1,&center);
  gp_Dir normal;
  ReadDir(arg2,&normal);

  if (!arg3->IsNumber())  {
    NanThrowError("expecting a number (radius) as third arguments");
    NanReturnUndefined();
  }
  double radius = arg3->ToNumber()->Value();
  if (radius<1E-9)  {
    NanThrowError("radius cannot be zero ( or close to zero)");
    NanReturnUndefined();
  }

  Edge* pThis = ObjectWrap::Unwrap<Edge>(args.This());

  pThis->createCircle(center,normal,radius);

  NanReturnValue(args.This());

}

NAN_METHOD(Edge::createArc3P)
{
  NanScope();


  Local<v8::Value> arg1 = args[0];
  Local<v8::Value> arg2 = args[1];
  Local<v8::Value> arg3 = args[2];

  if (arg1.IsEmpty()|| arg2.IsEmpty() || arg3.IsEmpty()) {
    NanThrowError("expecting three arguments : <center>,<normal>,<radius>");
    NanReturnUndefined();
  }

  Vertex* v1 =     getOrCreateVertex(arg1);
  gp_Pnt  p2;
  ReadPoint(arg2,&p2);
  Vertex* v3 =     getOrCreateVertex(arg3);

  Edge* pThis = ObjectWrap::Unwrap<Edge>(args.This());

  pThis->createArc3P(v1,v3,p2);

  NanReturnValue(args.This());
}


Persistent<FunctionTemplate> Edge::_template;

NAN_METHOD(Edge::New)
{
  NanScope();

  Edge* obj = new Edge();
  obj->Wrap(args.This());
  // return scope.Close(args.This());
  NanReturnValue(args.This());
}

Local<Object>  Edge::Clone() const
{

  Edge* obj = new Edge();
  Local<Object> instance = NanNew(_template)->GetFunction()->NewInstance();
  obj->Wrap(instance);
  obj->setShape(this->shape());
  return instance;
}
void Edge::Init(Handle<Object> target)
{
  // Prepare constructor template
  v8::Local<v8::FunctionTemplate> tpl = NanNew<v8::FunctionTemplate>(Edge::New);  
  tpl->SetClassName(NanNew("Edge"));

  // object has one internal filed ( the C++ object)
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  NanAssignPersistent<v8::FunctionTemplate>(_template, tpl);

  // Prototype
  Local<ObjectTemplate> proto = tpl->PrototypeTemplate();

  Base::InitProto(proto);

  EXPOSE_READ_ONLY_PROPERTY_DOUBLE(Edge,length);
  EXPOSE_READ_ONLY_PROPERTY_INTEGER(Edge,numVertices);
  EXPOSE_READ_ONLY_PROPERTY_BOOLEAN(Edge,isDegenerated);
  EXPOSE_READ_ONLY_PROPERTY_BOOLEAN(Edge,isClosed);
  EXPOSE_METHOD(Edge,createLine);
  EXPOSE_METHOD(Edge,createCircle);
  EXPOSE_METHOD(Edge,createArc3P);
  EXPOSE_METHOD(Edge,polygonize);

  target->Set(NanNew("Edge"), tpl->GetFunction());
}

Handle<Object> Edge::polygonize(double factor)
{

  const TopoDS_Edge& edge = TopoDS::Edge(this->shape());



  BRepAdaptor_Curve curve_adaptor(edge);
  GCPnts_UniformDeflection discretizer;
  discretizer.Initialize(curve_adaptor, 0.05);

  m_positions.clear();
  m_positions.reserve(discretizer.NbPoints()*3);

  for (int i = 0; i < discretizer.NbPoints(); i++) {

    gp_Pnt pt = curve_adaptor.Value(discretizer.Parameter(i + 1));

    //xx std::cerr  << "[ " << pt.X() << " " << pt.Y() << " " << pt.Z() << " ] ,";
    m_positions.push_back(static_cast<float>(pt.X()));
    m_positions.push_back(static_cast<float>(pt.Y()));
    m_positions.push_back(static_cast<float>(pt.Z()));
  } 
  //xx std::cerr << std::endl;
  Handle<Object> result = NanNew<Object>();

  int length = (int)m_positions.size();
  result->SetIndexedPropertiesToExternalArrayData(m_positions.data(), kExternalFloatArray,length);
  result->Set(NanNew("length"), NanNew<v8::Int32>(length));
  assert(     result->GetIndexedPropertiesExternalArrayDataLength() ==   length);
  assert(     result->Get(NanNew("length"))->ToInt32()->Uint32Value() ==   length);

  return result;
}


NAN_METHOD(Edge::polygonize)
{
  NanScope();
  // can work with this
  Handle<Object> pJhis = args.This();
  if ( pJhis.IsEmpty() || !NanHasInstance(_template,pJhis))  {
    // create a new object
    NanThrowError("invalid object");
  }
  Edge* pThis = node::ObjectWrap::Unwrap<Edge>(pJhis);

  double factor = 0.05;
  if (args.Length()>=1) {
    ReadDouble(args[0],factor);
  }

  NanReturnValue(pThis->polygonize(factor));
}
