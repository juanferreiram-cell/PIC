import 'package:flutter/material.dart';

class DrawingPage extends StatefulWidget {
  const DrawingPage({super.key});

  @override
  State<DrawingPage> createState() => _DrawingPageState();
}

class _DrawingPageState extends State<DrawingPage> {
  List<Offset?> points = [];

  void _addPoint(Offset localPosition) {
    if (localPosition.dx >= 0 &&
        localPosition.dx <= 240 &&
        localPosition.dy >= 0 &&
        localPosition.dy <= 320) {
      setState(() => points.add(localPosition));
    }
  }

  void _clearDrawing() {
    setState(() => points.clear());
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text("Lienzo 240x320")),
      backgroundColor: Colors.purple,
      body: Center(
        child: Container(
          color: Colors.black,
          width: 240,
          height: 320,
          child: Listener(
            onPointerMove: (event) => _addPoint(event.localPosition),
            onPointerUp: (_) => setState(() => points.add(null)),
            child: CustomPaint(
              painter: FreeDrawPainter(points),
              size: const Size(240, 320),
            ),
          ),
        ),
      ),
      floatingActionButton: FloatingActionButton(
        onPressed: _clearDrawing,
        tooltip: 'Borrar',
        child: const Icon(Icons.delete),
      ),
    );
  }
}

class FreeDrawPainter extends CustomPainter {
  final List<Offset?> points;
  FreeDrawPainter(this.points);

  @override
  void paint(Canvas canvas, Size size) {
    final paint = Paint()
      ..color = Colors.blue
      ..strokeWidth = 2
      ..strokeCap = StrokeCap.round;

    for (int i = 0; i < points.length - 1; i++) {
      if (points[i] != null && points[i + 1] != null) {
        canvas.drawLine(points[i]!, points[i + 1]!, paint);
      }
    }
  }

  @override
  bool shouldRepaint(covariant FreeDrawPainter oldDelegate) => true;
}
